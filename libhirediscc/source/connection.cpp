//---------------------------------------------------------------------------------------------------------------------
// Copyright (c) 2016 libhirediscc project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//---------------------------------------------------------------------------------------------------------------------

#include <WinSock2.h>
#include <hiredis.h>

#include <hirediscc/exception.h>
#include <hirediscc/reply.h>
#include <hirediscc/connection.h>

namespace hirediscc {

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

Connection::Connection() {
}

Connection::~Connection() {
	//Trace("Connection closed\n");
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

}
