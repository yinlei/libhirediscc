//---------------------------------------------------------------------------------------------------------------------
// Copyright (c) 2016 libhirediscc project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//---------------------------------------------------------------------------------------------------------------------

#include <cassert>
#include <hirediscc/connection.h>
#include <hirediscc/reply.h>
#include <hirediscc/client.h>

namespace hirediscc {

Client::Client(std::string const & host, uint16_t port, std::string const & password) {
    connection_ = std::make_unique<Connection>();
    connection_->connect(host, port);
    if (!password.empty())
        connection_->setAuth(password);
}

void Client::quit() {
    connection_->excuteCommandWithArgs<ReplyString>("QUIT").value();
}

Pipelined Client::pipelined() {
	return Pipelined(connection_);
}

Client::Client(ConnectionPtr conn)
    : connection_(conn) {
    assert(conn != nullptr);
}

Client::~Client() {
}

std::string Client::get(std::string const & key) {
    return connection_->excuteCommandWithArgs<ReplyString>("GET", key).value();
}

std::vector<std::string> Client::keys() {
    return connection_->excuteCommandWithArgs<ReplyArray<ReplyString>>("KEYS", "*").value();
}

}
