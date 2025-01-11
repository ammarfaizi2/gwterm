// SPDX-License-Identifier: GPL-2.0-only

#ifndef EXC__EXCHANGE_FOUNDATION__HPP
#define EXC__EXCHANGE_FOUNDATION__HPP

#include <string>
#include <mutex>
#include <memory>
#include <functional>
#include <unordered_map>

#include <wbx/exc/Websocket.hpp>

namespace wbx {
namespace exc {

struct ExcPriceUpdate {
	std::string	symbol;
	std::string	price;
	uint64_t	ts;
};

class ExchangeFoundation;

typedef std::function<void(ExchangeFoundation *ef, const ExcPriceUpdate &up, void *udata)> PriceUpdateCb_t;

struct PriceUpdateCbData {
	PriceUpdateCb_t	cb;
	void		*udata;
};

struct OHLCPrice {
	uint64_t	ts_last;
	uint64_t	ts_open;
	uint64_t	ts_close;

	uint64_t	open;
	uint64_t	high;
	uint64_t	low;
	uint64_t	close;

	uint64_t	curr;
	uint64_t	prev;

	uint64_t	prec;
};

struct OHLCData {
	uint64_t			nr_samples = 0;
	std::vector<struct OHLCPrice>	prices;
};

struct OHLCGroup {
	struct OHLCData ohlc_1s_;
	struct OHLCData ohlc_1m_;
	struct OHLCData ohlc_5m_;
	struct OHLCData ohlc_15m_;
	struct OHLCData ohlc_30m_;
	struct OHLCData ohlc_1h_;
	struct OHLCData ohlc_4h_;
};

class ExchangeFoundation {
private:
	std::mutex m_price_update_cbs_mtx_;
	std::unordered_map<std::string, PriceUpdateCbData> m_price_update_cbs_;

	std::mutex m_last_prices_mtx_;
	std::unordered_map<std::string, uint64_t> m_last_prices_;
	std::unordered_map<std::string, uint64_t> m_precisions_;
	std::unordered_map<std::string, struct OHLCGroup> m_ohlc_data_;

protected:
	std::shared_ptr<Websocket> ws_ = nullptr;
	WebsocketSession *wss_ = nullptr;

	void addPriceUpdateCb(const std::string &symbol, PriceUpdateCb_t cb, void *udata = nullptr);
	void delPriceUpdateCb(const std::string &symbol);
	void invokePriceUpdateCb(const ExcPriceUpdate &up);
	static void __setOHLCData(struct OHLCData &dt, uint64_t price, uint64_t prec, uint64_t ts, uint64_t tsec);
	void __setOHLCGroup(const std::string &symbol, uint64_t price, uint64_t prec, uint64_t ts);
	void setLastPrice(const std::string &symbol, const std::string &price, uint64_t ts = 0);
	void delLastPrice(const std::string &symbol);
	virtual void onWsConnect(void) = 0;
	virtual void onWsWrite(size_t len) = 0;
	virtual size_t onWsRead(const char *data, size_t len) = 0;
	virtual void onWsClose(void) = 0;

public:
	ExchangeFoundation(void);
	~ExchangeFoundation(void);

	virtual void listenPriceUpdate(const std::string &symbol, PriceUpdateCb_t cb, void *udata) = 0;
	virtual void unlistenPriceUpdate(const std::string &symbol) = 0;
	virtual void start(void) = 0;
	virtual std::string getLastPrice(const std::string &symbol);

	void setWebsocket(std::shared_ptr<Websocket> ws);

	static std::string formatPrice(uint64_t price, uint64_t precision);
	void dumpOHLCData(const std::string &symbol);
};

} /* namespace exc */
} /* namespace wbx */

#endif /* #ifndef EXC__EXCHANGE_FOUNDATION__HPP */
