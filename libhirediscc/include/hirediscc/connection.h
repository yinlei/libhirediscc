//---------------------------------------------------------------------------------------------------------------------
// Copyright (c) 2016 libhirediscc project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//---------------------------------------------------------------------------------------------------------------------

#pragma once

#include <memory>

#include <hirediscc/details.h>
#include <hirediscc/commandargs.h>

namespace hirediscc {

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

}
