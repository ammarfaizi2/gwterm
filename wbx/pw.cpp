// SPDX-License-Identifier: GPL-2.0-only

#include <iostream>
#include <wbx/exc/exc_okx/OKX.hpp>
#include <wbx/exc/exc_binance/Binance.hpp>

using namespace wbx::exc;

using wbx::exc::Websocket;
using wbx::exc::ExchangeFoundation;
using wbx::exc::exc_OKX::OKX;
using wbx::exc::exc_Binance::Binance;

static const char *symbols[] = {
	"BTC-USDT",
	"ETH-USDT",
	"BNB-USDT",
	"SOL-USDT",
	"XRP-USDT",
	"TON-USDT",
	"NOT-USDT",
	"AAVE-USDT",
};

template <typename T>
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
	printf("| %s | %s |", date, static_cast<const char *>(udata));

	for (i = 0; i < sizeof(symbols) / sizeof(symbols[0]); i++) {
		std::string sym = symbols[i];
		// Remove "-USDT" from the symbol
		sym.erase(sym.size() - 5);

		if (prices[i].empty())
			prices[i] = "N/A";
		if (directions[i] == 1)
			printf(" %s%s: %s%s |", tgreen, sym.c_str(), prices[i].c_str(), "\033[0m");
		else if (directions[i] == -1)
			printf(" %s%s: %s%s |", tred, sym.c_str(), prices[i].c_str(), "\033[0m");
		else
			printf(" %s%s%s: %s |", tcyan, sym.c_str(), "\033[0m", prices[i].c_str());
	}

	printf("\n");
	(void)udata;
}

int main(void)
{
	std::shared_ptr<Websocket> ws = std::make_shared<Websocket>();
	std::unique_ptr<Binance> bnb = std::make_unique<Binance>(ws);
	std::unique_ptr<OKX> okx = std::make_unique<OKX>(ws);

	{
		std::vector<std::string> symbols_v(symbols, symbols + sizeof(symbols) / sizeof(symbols[0]));
		char *p;

		bnb->start();
		p = (char *)"binance ";
		bnb->listenPriceUpdateBatch(symbols_v, [](ExchangeFoundation *binance, const ExcPriceUpdate &up, void *udata) {
			price_update_cb<Binance>(binance, up, udata);
		}, p);

		okx->start();
		p = (char *)"okx     ";
		okx->listenPriceUpdateBatch(symbols_v, [](ExchangeFoundation *okx, const ExcPriceUpdate &up, void *udata) {
			price_update_cb<OKX>(okx, up, udata);
		}, p);
	}

	ws->run();
	return 0;
}
