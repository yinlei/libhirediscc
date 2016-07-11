//---------------------------------------------------------------------------------------------------------------------
// Copyright (c) 2016 libhirediscc project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//---------------------------------------------------------------------------------------------------------------------

#pragma once

#include <string>
#include <vector>
#include <memory>

#include <hirediscc/pipelined.h>
#include <hirediscc/commandargs.h>

namespace hirediscc {

class Connection;
using ConnectionPtr = std::shared_ptr<Connection>;

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

	Pipelined pipelined();

private:
	ConnectionPtr connection_;
};

template <typename T>
inline void Client::set(std::string const & key, T value) {
	connection_->excuteCommandWithArgs<ReplyString>("SET", key, value);
}

template <typename T>
inline std::string Client::echo(T message) {
	return connection_->excuteCommandWithArgs<ReplyString>("ECHO", message).value();
}

template <typename T, typename... Args>
inline void Client::del(T arg, Args const &... args) {
	connection_->excuteCommandWithArgs<ReplyInterger>("DEL", arg, args ...);
}

}