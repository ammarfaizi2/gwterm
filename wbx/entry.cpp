// SPDX-License-Identifier: GPL-2.0-only

#include <iostream>
#include <wbx/exc/Websocket.hpp>

static const char payload[] = "{\"op\":\"subscribe\",\"args\":[{\"channel\":\"mark-price\",\"instId\":\"BTC-USDT\"}, {\"channel\":\"mark-price\",\"instId\":\"ETH-USDT\"}]}";

int main(void)
{
	try {
		wbx::exc::Websocket ws;

		auto *wsk = ws.createSession("wspri.okx.com", 8443, "/ws/v5/ipublic");
		wsk->setOnConnect([](wbx::exc::WebsocketSession *wsk) {
			std::cout << "Connected!" << std::endl;
			wsk->write(payload, sizeof(payload) - 1);
		});

		wsk->setOnWrite([](wbx::exc::WebsocketSession *wsk, size_t len) {
			std::cout << "Written: " << len << std::endl;
			wsk->read();
		});

		wsk->setOnRead([](wbx::exc::WebsocketSession *wsk, const char *data, size_t len) {
			std::cout << "Read: " << std::string(data, len) << std::endl;
			wsk->readAfter();
			return len;
		});

		wsk->setOnClose([](wbx::exc::WebsocketSession *wsk) {
			std::cout << "Closed!" << std::endl;
		});

		wsk->run();
		ws.run();
	} catch (const std::exception &e) {
		std::cerr << "Exception: " << e.what() << std::endl;
		throw e;
	}
}
