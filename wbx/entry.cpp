// SPDX-License-Identifier: GPL-2.0-only

#include <iostream>
#include <wbx/exc/exc_okx/OKX.hpp>

using namespace wbx::exc;

using wbx::exc::Websocket;
using wbx::exc::ExchangeFoundation;
using wbx::exc::exc_OKX::OKX;

static const char *symbols[] = {
	"BTC-USDT",
	"ETH-USDT",
	"SOL-USDT",
	"BNB-USDT",
	"TON-USDT",
	"NOT-USDT",
	"MEW-USDT",
	"XAUT-USDT",
};


static void price_update_cb(ExchangeFoundation *okx, const ExcPriceUpdate &up, void *udata)
{
	static const char tred[] = "\033[31m";
	static const char tgreen[] = "\033[32m";
	static const char tcyan[] = "\033[36m";

	static char directions[sizeof(symbols) / sizeof(symbols[0])] = { 0 };
	static std::string prev_prices[sizeof(symbols) / sizeof(symbols[0])];
	std::string prices[sizeof(symbols) / sizeof(symbols[0])];
	bool changed = false;
	size_t i;

	for (i = 0; i < sizeof(symbols) / sizeof(symbols[0]); i++) {
		prices[i] = okx->getLastPrice(symbols[i]);
		directions[i] = 0;

		if (prices[i] == prev_prices[i])
			continue;

		changed = true;
		if (prices[i] > prev_prices[i])
			directions[i] = 1;
		else
			directions[i] = -1;

		prev_prices[i] = prices[i];
	}

	if (!changed)
		return;

	char date[32];
	time_t t = (up.ts / 1000) + (3600 * 7);
	struct tm *tm = gmtime(&t);

	if (!tm)
		return;

	strftime(date, sizeof(date), "%Y-%m-%d %H:%M:%S", tm);
	printf("| %s |", date);

	for (i = 0; i < sizeof(symbols) / sizeof(symbols[0]); i++) {
		if (prices[i].empty())
			prices[i] = "N/A";
		if (directions[i] == 1)
			printf(" %s%s: %s%s |", tgreen, symbols[i], prices[i].c_str(), "\033[0m");
		else if (directions[i] == -1)
			printf(" %s%s: %s%s |", tred, symbols[i], prices[i].c_str(), "\033[0m");
		else
			printf(" %s%s%s: %s |", tcyan, symbols[i], "\033[0m", prices[i].c_str());
	}

	printf("\n");
	(void)udata;

	okx->getLastPrice("BCH-USDT", [](const std::string &price) {
		std::cout << "BCH-USDT: " << price << std::endl;
	});
}

int main(void)
{
	std::shared_ptr<Websocket> ws = std::make_unique<Websocket>();
	std::unique_ptr<OKX> okx = std::make_unique<OKX>();

	okx->setWebsocket(ws);
	okx->start();

	{
		std::vector<std::string> symbols_v(symbols, symbols + sizeof(symbols) / sizeof(symbols[0]));
		okx->listenPriceUpdateBatch(symbols_v, price_update_cb, nullptr);
	}

	ws->run();
	return 0;
}
