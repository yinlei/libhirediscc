//---------------------------------------------------------------------------------------------------------------------
// Copyright (c) 2016 libhirediscc project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//---------------------------------------------------------------------------------------------------------------------

#include <hirediscc/exception.h>
#include <hirediscc/connection.h>
#include <hirediscc/connectionpool.h>

namespace hirediscc {

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
                    //Trace("Ping connection!\n");
                    if (conn->ping() == "PONG") {
                        if (idleCount_ < configuration_.maxIdle) {
                            ++idleCount_;
                            aliveConn_.try_enqueue(conn);
                        }
                    } else {
                        poolConn_.try_enqueue(conn);
                    }
                } catch (Exception const &e) {
                    //Trace("Connection closed (reson:%s)\n", e.what());
                    conn->close();
                    conn->connect(configuration_.host, configuration_.port);
                    poolConn_.try_enqueue(conn);
                }
            }

            //Trace("Connection pool: %d, alive: %d\n", capacity, idleCount_);
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
