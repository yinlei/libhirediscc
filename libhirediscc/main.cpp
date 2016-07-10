#include <iostream>
#include <hirediscc/hirediscc.h>

void pingTest() {
	hirediscc::Connection connection;
	connection.connect("127.0.0.1", 6379);
	for (int i = 0; i < 1000; ++i) {
		connection.ping();
	}
}

void conntectTimeoutTest() {
	try {
		hirediscc::Connection connection;
		connection.connect("127.0.0.1", 1234, 10);
	} catch (hirediscc::Exception const &e) {
		std::cerr << e.what();
	}
}

void arrayTest() {
	hirediscc::Connection connection;
	connection.connect("127.0.0.1", 6379);
	connection.excute<hirediscc::ReplyArray<hirediscc::ReplyString>>("KEYS", "*");
}

void clientSetGetTest() {
	hirediscc::Client client("127.0.0.1", 6379);
	client.set("mykey", "Hello");
	auto value = client.get("mykey");
	auto keys = client.keys();
	client.del("mykey");
}

void clientEcho() {
	hirediscc::Client client("127.0.0.1", 6379);
	client.echo("Hello");
}

void connectionPoolTest() {
	hirediscc::ConnectionPool connPool({
		1024,
		4096,
		100,
		100,
		100,
		"127.0.0.1",
		6379
	});

	using namespace std::this_thread;
	using namespace std::chrono;

	std::vector<std::thread> pool;

	for (int i = 0; i < 10; ++i) {
		pool.emplace_back([&]() {
			for (int i = 0; i < 10; ++i) {
				try {
					hirediscc::Client client(connPool.borrowConnection());
					client.echo("Hello");
				} catch (hirediscc::Exception const &e) {
					std::cerr << e.what();
				}
			}
		});
		sleep_for(milliseconds(1 + (rand() % 100)));
	}

	for (int i = 0; i < pool.size(); ++i)
		pool[i].join();
}

int main() {
	pingTest();
	conntectTimeoutTest();
	arrayTest();
	clientSetGetTest();
	clientEcho();
	//connectionPoolTest();
}