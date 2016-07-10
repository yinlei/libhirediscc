//---------------------------------------------------------------------------------------------------------------------
// Copyright (c) 2016 libhirediscc project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//---------------------------------------------------------------------------------------------------------------------

#pragma once

#include <cassert>
#include <cstdint>
#include <memory>
#include <atomic>
#include <string>
#include <thread>

#include <hirediscc/mpmc_bounded_queue.h>

namespace hirediscc {

class Connection;
using ConnectionPtr = std::shared_ptr<Connection>;

class Context;

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

}

