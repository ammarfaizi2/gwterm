// SPDX-License-Identifier: GPL-2.0-only

#include <iostream>
#include <wbx/exc/exc_okx/OKX.hpp>

using namespace wbx::exc;

using wbx::exc::Websocket;
using wbx::exc::ExchangeFoundation;
using wbx::exc::exc_OKX::OKX;

static void btc_usdt_cb(ExchangeFoundation *ef, const price_update &up, void *udata)
{
	printf("symbol: %s, ts: %lu, price: %s\n", up.symbol.c_str(), up.ts, up.price.c_str());
}

int main(void)
{
	std::shared_ptr<Websocket> ws = std::make_unique<Websocket>();
	std::unique_ptr<OKX> okx = std::make_unique<OKX>();

	okx->setWebsocket(ws);
	okx->start();
	okx->listenPriceUpdate("ETH-USDT", &btc_usdt_cb, nullptr);
	ws->run();
	return 0;
}
