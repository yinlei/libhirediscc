//---------------------------------------------------------------------------------------------------------------------
// Copyright (c) 2016 libhirediscc project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//---------------------------------------------------------------------------------------------------------------------

#pragma once

#include <string>
#include <cstdint>
#include <memory>
#include <vector>
#include <cassert>
#include <atomic>
#include <thread>
#include <stdexcept>

#include "mpmc_bounded_queue.h"

struct redisReply;
struct redisContext;

namespace hirediscc {

class Connection;
using ConnectionPtr = std::shared_ptr<Connection>;

class Context;

namespace details {
    void deleteRedisReply(redisReply *reply);
    int32_t getRedisReplyType(redisReply *reply);
    void deserializeRedisReply(redisReply *reply, std::string &result);
    void deserializeRedisReply(redisReply *reply, int64_t &result);
    void deserializeRedisReply(redisReply *reply, std::vector<redisReply*> &result);
    redisReply *excute(redisContext* context);
}

class Exception : public std::exception {
public:
    Exception() = default;

    explicit Exception(int error);

    virtual char const * what() const override;
private:
    int error_;
};

class ConnectionPool {
public:
    struct Configuration {
        uint32_t initCapacity;
        uint32_t maxCapacity;
        uint32_t capacityIncrement;
        uint32_t maxIdleTimeout;
        uint32_t maxIdle;
        std::string host;
        uint16_t port;
        std::string password;
    };

    class ConnectionPtrDeleter {
    public:
        ConnectionPtrDeleter() {
			assert(0 && "Can't construcotr default");
		}

        explicit ConnectionPtrDeleter(std::weak_ptr<ConnectionPool*> pool)
            : pool_(pool) {}

        void operator()(Connection* p) {
            auto pool = pool_.lock();
            if (!pool) {
                std::default_delete<Connection>{}(p);
                return;
            }
            try {
                (*pool.get())->returnConnection(
                    std::unique_ptr<Connection, ConnectionPtrDeleter>{p, ConnectionPtrDeleter(pool)});
            } catch (...) { }
        }
    private:
        std::weak_ptr<ConnectionPool*> pool_;
    };

    using PoolablesConnectionPtr = std::unique_ptr<Connection, ConnectionPtrDeleter>;

    explicit ConnectionPool(Configuration configuration);

    ~ConnectionPool();

    ConnectionPtr borrowConnection();

    void returnConnection(ConnectionPtr conn);

private:
    PoolablesConnectionPtr allocate();

    Configuration configuration_;
    volatile bool stopped_;
    std::atomic<uint32_t> idleCount_;
    std::shared_ptr<ConnectionPool*> this_;
    mpmc_bounded_queue<ConnectionPtr> poolConn_;
    mpmc_bounded_queue<ConnectionPtr> aliveConn_;
    std::thread thread_;
};

class CommandArgs {
public:
    using Iterator = std::vector<std::string>::const_iterator;

    CommandArgs() = default;

    explicit CommandArgs(std::string const &arg) {
        argsList_.push_back(arg);
    }

    template <typename T>
    CommandArgs& operator<<(T const &arg) {
        argsList_.push_back(std::to_string(arg));
        return *this;
    }

    CommandArgs& operator<<(std::string const &arg) {
        argsList_.push_back(arg);
        return *this;
    }

    CommandArgs& operator<<(char const *arg) {
        argsList_.push_back(arg);
        return *this;
    }

    CommandArgs& operator<<(CommandArgs const &commandArgs) {
        argsList_.reserve(count() + commandArgs.count());
        argsList_.insert(std::end(argsList_),
            std::begin(commandArgs.argsList_),
            std::end(commandArgs.argsList_));
        return *this;
    }

    size_t count() const {
        return argsList_.size();
    }

    Iterator begin() const {
        return std::begin(argsList_);
    }

    Iterator end() const {
        return std::end(argsList_);
    }

private:
    std::vector<std::string> argsList_;
};

#pragma region Reply

template <typename T, typename R>
class ReplyBase {
public:
    static ReplyBase<T, R> const NullType;

    enum Type {
        String = 1,
        Array,
        Interger,
        Null,
        Status,
        Error
    };

    ReplyBase()
        : type_(Null)
        , reply_(nullptr) {
    }

    ReplyBase(Type type)
        : type_(type)
        , reply_(nullptr) {
    }

    explicit ReplyBase(redisReply *reply)
        : type_(Null)
        , reply_(reply) {
        type_ = static_cast<Type>(details::getRedisReplyType(reply));
    }

    ~ReplyBase() {
        details::deleteRedisReply(reply_);
    }

    R value() {
        return static_cast<T*>(this)->value();
    }

    void deserialize(redisReply *reply) {
        return static_cast<T*>(this)->deserialize(reply);
    }

    bool hasError() const noexcept {
        return false;
    }

    bool isInterger() const noexcept {
        return type_ == Interger;
    }

    bool isError() const noexcept {
        return type_ == Error;
    }

    bool isString() const noexcept {
        return type_ == String;
    }

    bool isStatus() const noexcept {
        return type_ == Status;
    }
protected:
    redisReply *reply_;
    Type type_;
};

template <typename T, typename R>
ReplyBase<T, R> const ReplyBase<T, R>::NullType;

class ReplyInterger : public ReplyBase<ReplyInterger, int64_t> {
public:
    ReplyInterger()
        : ReplyBase() {
    }

    explicit ReplyInterger(redisReply *reply)
        : ReplyBase(reply) {
        deserialize(reply);
    }

    ReplyInterger(ReplyInterger &&other) {
        *this = std::move(other);
    }

    ReplyInterger& operator=(ReplyInterger &&other) {
        if (this != &other) {
            reply_ = other.reply_;
            type_ = other.type_;
            value_ = other.value_;
            other.reply_ = nullptr;
            other.type_ = Null;
            other.value_ = 0;
        }
        return *this;
    }

    int64_t value() const {
        return value_;
    }

    void deserialize(redisReply *reply) {
        assert(isInterger());
        details::deserializeRedisReply(reply, value_);
    }

private:
    int64_t value_;
};

class ReplyString : public ReplyBase<ReplyString, std::string> {
public:
    using ValueType = std::string;

    ReplyString()
        : ReplyBase(String) {
    }

    explicit ReplyString(redisReply *reply)
        : ReplyBase(reply) {
        deserialize(reply);
    }

    ReplyString(ReplyString &&other) {
        *this = std::move(other);
    }

    ReplyString& operator=(ReplyString &&other) {
        if (this != &other) {
            reply_ = other.reply_;
            type_ = other.type_;
            value_ = std::move(other.value_);
            other.reply_ = nullptr;
            other.type_ = Null;
        }
        return *this;
    }

    std::string value() const {
        return value_;
    }

    void deserialize(redisReply *reply) {
        assert(isString() || isError() || isStatus());
        details::deserializeRedisReply(reply, value_);
    }
private:
    std::string value_;
};

template <typename T>
class ReplyArray : public ReplyBase<ReplyArray<T>, std::vector<T>> {
public:
    ReplyArray()
        : ReplyBase(Array) {
    }

    explicit ReplyArray(redisReply *reply)
        : ReplyBase(reply) {
        deserialize(reply);
    }

    ReplyArray(ReplyArray &&other) {
        *this = std::move(other);
    }

    ReplyArray& operator=(ReplyArray &&other) {
        if (this != &other) {
            reply_ = other.reply_;
            type_ = other.type_;
            value_ = std::move(other.value_);
            other.reply_ = nullptr;
            other.type_ = Null;
        }
        return *this;
    }

    std::vector<typename T::ValueType> value() const {
        std::vector<typename T::ValueType> values;
        values.reserve(value_.size());
        for (auto &v : value_)
            values.push_back(v.value());
        return values;
    }

    void deserialize(redisReply *reply) {
        std::vector<redisReply*> result;
        details::deserializeRedisReply(reply, result);

        auto itr = result.begin();
        value_.resize(result.size());

        for (auto &v : value_) {
            v.deserialize(*itr);
            ++itr;
        }
    }
private:
    std::vector<T> value_;
};
#pragma endregion Reply

class Context {
public:
    explicit Context(redisContext *context);

    Context(Context const &) = delete;
    Context& operator=(Context const &) = delete;

    ~Context();

    friend Context& operator<<(Context &context, CommandArgs const &args);

    void appendCommandArgs(CommandArgs const &args);

    void enableKeepAlive();

    template <typename T>
    T excute() {
        T reply(details::excute(context_));
        return std::move(reply);
    }
private:
    redisContext *context_;
};

class Connection {
public:
    enum {
        DefaultTimeout = 5
    };

    Connection();

    ~Connection();

    Connection(Connection const &) = delete;
    Connection& operator=(Connection const &) = delete;

    void connect(std::string const &host,
        uint16_t port,
        int timeout = DefaultTimeout);

    void close();

    std::string ping();

    std::string setAuth(std::string const &password);

    template <typename R, typename T, typename... Args>
    R excute(T arg, Args const &... args) {
        CommandArgs commandArgs;
        append(commandArgs, arg, args...);
        context_->appendCommandArgs(commandArgs);
        return context_->excute<R>();
    }

    template <typename T>
    void append(CommandArgs &commandArgs, T arg) {
        commandArgs << arg;
    }

    template <typename T, typename... Args>
    void append(CommandArgs &commandArgs, T arg, Args const &... args) {
        commandArgs << arg;
        append(commandArgs, args ...);
    }

private:
    std::unique_ptr<Context> context_;
};

class Client {
public:
    Client(std::string const &host,
        uint16_t port,
        std::string const &password = "");

    explicit Client(ConnectionPtr conn);

    ~Client();

    template <typename T>
    void set(std::string const &key, T value);

    template <typename T>
    std::string echo(T message);

    std::string get(std::string const &key);

    std::vector<std::string> keys();

    template <typename T, typename... Args>
    void del(T arg, Args const &... args);

    void quit();
    
private:
    ConnectionPtr connection_;
};

template <typename T>
inline void Client::set(std::string const & key, T value) {
    connection_->excute<ReplyString>("SET", key, value);
}

template<typename T>
inline std::string Client::echo(T message) {
    return connection_->excute<ReplyString>("ECHO", message).value();
}

template<typename T, typename... Args>
inline void Client::del(T arg, Args const &... args) {
    connection_->excute<ReplyInterger>("DEL", arg, args ...);
}

}

