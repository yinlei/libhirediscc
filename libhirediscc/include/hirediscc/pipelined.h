//---------------------------------------------------------------------------------------------------------------------
// Copyright (c) 2016 libhirediscc project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//---------------------------------------------------------------------------------------------------------------------

#pragma once

#include <memory>

#include <hirediscc/connection.h>

namespace hirediscc {

class Connection;
using ConnectionPtr = std::shared_ptr<Connection>;

class Pipelined {
public:
	explicit Pipelined(ConnectionPtr conn);

	template <typename T, typename... Args>
	void add(T arg, Args const &... args);

	void excute();

	template <typename R>
	R get();
private:
	CommandArgs commandArgs_;
	ConnectionPtr connection_;
};

template<typename T, typename ...Args>
inline void Pipelined::add(T arg, Args const & ...args) {
	connection_->append(commandArgs_, arg, args...);
}

template <typename R>
inline R Pipelined::get() {
	return connection_->excuteOnce<R>();
}

}
