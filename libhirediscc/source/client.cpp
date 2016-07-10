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

}
