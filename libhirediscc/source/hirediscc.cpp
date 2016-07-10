//---------------------------------------------------------------------------------------------------------------------
// Copyright (c) 2016 libhirediscc project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//---------------------------------------------------------------------------------------------------------------------

#include <WinSock2.h>

#include <unordered_map>
#include <hiredis.h>
#include <hirediscc/hirediscc.h>

#if defined(_DEBUG)
void Trace(char const *format, ...) {
    va_list args;
    va_start(args, format);
    // Get format buffer size.
    int bufSize = _vscprintf(format, args) + 1;
    std::vector<char> buffer(bufSize);
    // Do printf.
    _vsnprintf_s(buffer.data(), bufSize, bufSize - 1, format, args);
    va_end(args);
    ::OutputDebugStringA(buffer.data());
}
#else
#define Trace(...)
#endif

namespace hirediscc {

Exception::Exception(int error)
    : error_(error){
}

char const * Exception::what() const {
    static std::unordered_map<int, std::string> const lookupTable {
        { 
            REDIS_ERR_IO,
            "There was an I/O error while creating the connection, "
            "trying to write to the socket or read from the socket."
        },
        {
            REDIS_ERR_EOF,
            "The server closed the connection which resulted in an empty read."
        },
        {
            REDIS_ERR_OTHER, "Any other error."
        },
        { 
            REDIS_ERR_PROTOCOL,
            "There was an error while parsing the protocol."
        }
    };
    auto itr = lookupTable.find(error_);
    if (itr != lookupTable.end())
        return (*itr).second.c_str();
    return "";
}

Context::Context(redisContext *context)
    : context_(context) {
}

Context::~Context() {
    if (context_)
        ::redisFree(context_);
}

Context& operator<<(Context &context,
    CommandArgs const &args) {
    context.appendCommandArgs(args);
    return context;
}

void Context::appendCommandArgs(CommandArgs const & args) {
    std::vector<char const*> argv;
    argv.reserve(args.count());

    std::vector<size_t> argvlen;
    argvlen.reserve(args.count());

    for (auto const & itr : args) {
        argv.push_back(itr.c_str());
        argvlen.push_back(itr.size());
    }

    auto ret = ::redisAppendCommandArgv(context_,
        static_cast<int>(args.count()),
        &argv[0], 
        argvlen.data());
    if (ret != REDIS_OK) {
        throw Exception(ret);
    }
}

void Context::enableKeepAlive() {
    auto ret = ::redisEnableKeepAlive(context_);
    if (ret != REDIS_OK) {
        throw Exception(ret);
    }
}

namespace details {

void deleteRedisReply(redisReply * reply) {
    if (reply != nullptr)
        ::freeReplyObject(reply);
}

int32_t getRedisReplyType(redisReply *reply) {
    return int32_t(reply->type);
}

void deserializeRedisReply(redisReply * reply, std::string &result) {
    result.assign(reply->str, reply->len);
}

void deserializeRedisReply(redisReply * reply, int64_t &result) {
    result = reply->integer;
}

void deserializeRedisReply(redisReply * reply, std::vector<redisReply*> &result) {
    result.reserve(reply->elements);
    for (size_t i = 0; i < reply->elements; ++i)
        result.push_back(reply->element[i]);
}

redisReply * excute(redisContext * context) {
    redisReply *r = nullptr;
    auto ret = ::redisGetReply(context, reinterpret_cast<void**>(&r));
    if (ret != REDIS_OK) {
        throw Exception(ret);
    }
    return r;
}

}

Connection::Connection() {
}

Connection::~Connection() {
    Trace("Connection closed\n");
}

void Connection::connect(std::string const &host, uint16_t port, int timeout) {
    struct timeval timeoutSetting;
    timeoutSetting.tv_sec = timeout;
    timeoutSetting.tv_usec = 0;
    auto ctx = ::redisConnectWithTimeout(host.c_str(), port, timeoutSetting);
    if (!ctx || ctx->err) {
        throw Exception(ctx->err);
    }
    context_ = std::make_unique<Context>(ctx);
    context_->enableKeepAlive();
}

void Connection::close() {
    context_.reset();
}

std::string Connection::ping() {
    return excute<ReplyString>("PING").value();
}

std::string Connection::setAuth(std::string const &password) {
    return excute<ReplyString>("AUTH", password).value();
}

Client::Client(std::string const & host, uint16_t port, std::string const & password) {
    connection_ = std::make_unique<Connection>();
    connection_->connect(host, port);
    if (!password.empty())
        connection_->setAuth(password);
}

void Client::quit() {
    connection_->excute<ReplyString>("QUIT").value();
}

Client::Client(ConnectionPtr conn)
    : connection_(conn) {
    assert(conn != nullptr);
}

Client::~Client() {
}

std::string Client::get(std::string const & key) {
    return connection_->excute<ReplyString>("GET", key).value();
}

std::vector<std::string> Client::keys() {
    return connection_->excute<ReplyArray<ReplyString>>("KEYS", "*").value();
}

ConnectionPool::ConnectionPool(Configuration configuration) 
    : configuration_(configuration)
    , stopped_(false)
    , idleCount_(0)
    , this_(new ConnectionPool*(this))
    , poolConn_(configuration_.maxCapacity)
    , aliveConn_(configuration_.maxCapacity) {

    uint32_t capacity = 0;

    for (uint32_t i = 0; i < configuration_.initCapacity; ++i) {
        ConnectionPtr conn(allocate());
        conn->connect(configuration_.host, configuration_.port);
        poolConn_.try_enqueue(conn);
        ++capacity;
    }

    auto handler = [this, capacity]() mutable {
        using namespace std::chrono;
        using namespace std::this_thread;

        while (!stopped_) {
            ConnectionPtr conn;

            if (!poolConn_.try_dequeue(conn)) {
                if (capacity < configuration_.maxCapacity) {
                    ConnectionPtr newConn(allocate());
                    newConn->connect(configuration_.host, configuration_.port);
                    poolConn_.try_enqueue(newConn);
                    ++capacity;
                } else {
                    poolConn_.try_enqueue(conn);
                }
            } else {
                try {
                    Trace("Ping connection!\n");
                    if (conn->ping() == "PONG") {
                        if (idleCount_ < configuration_.maxIdle) {
                            ++idleCount_;
                            aliveConn_.try_enqueue(conn);
                        }
                    } else {
                        poolConn_.try_enqueue(conn);
                    }
                } catch (Exception const &e) {
                    Trace("Connection closed (reson:%s)\n", e.what());
                    conn->close();
                    conn->connect(configuration_.host, configuration_.port);
                    poolConn_.try_enqueue(conn);
                }
            }

            Trace("Connection pool: %d, alive: %d\n", capacity, idleCount_);
            sleep_for(milliseconds(configuration_.maxIdleTimeout));
        }
    };

    thread_ = std::thread(handler);
}

ConnectionPool::~ConnectionPool() {
    stopped_ = true;
    if (thread_.joinable())
        thread_.join();
}

ConnectionPtr ConnectionPool::borrowConnection() {
    ConnectionPtr conn;
    do {
        aliveConn_.try_dequeue(conn);
    } while (conn == nullptr);
    --idleCount_;
    return conn;
}

void ConnectionPool::returnConnection(ConnectionPtr conn) {
    poolConn_.try_enqueue(conn);
}

ConnectionPool::PoolablesConnectionPtr ConnectionPool::allocate() {
    return PoolablesConnectionPtr(new Connection(), ConnectionPtrDeleter{
        std::weak_ptr<ConnectionPool*>{this_}
    });
}

}
