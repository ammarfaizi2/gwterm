// SPDX-License-Identifier: GPL-2.0-only

#ifndef EXC__EXCHANGE_FOUNDATION__HPP
#define EXC__EXCHANGE_FOUNDATION__HPP

#include <string>
#include <mutex>
#include <queue>
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
	constexpr static uint64_t	max_samples = 4096;
	std::vector<struct OHLCPrice>	prices;
};

struct OHLCGroup {
	struct OHLCData ohlc_1s;
	struct OHLCData ohlc_1m;
	struct OHLCData ohlc_5m;
	struct OHLCData ohlc_15m;
	struct OHLCData ohlc_30m;
	struct OHLCData ohlc_1h;
	struct OHLCData ohlc_4h;
	struct OHLCData ohlc_1d;
};

class ExchangeFoundation {
private:
	std::mutex m_price_update_cbs_mtx_;
	std::unordered_map<std::string, PriceUpdateCbData> m_price_update_cbs_;
	std::unordered_map<std::string, std::queue<PriceUpdateCb_t>> m_get_last_price_cbs_;

	std::mutex m_last_prices_mtx_;
	std::unordered_map<std::string, uint64_t> m_last_prices_;
	std::unordered_map<std::string, uint64_t> m_precisions_;
	std::unordered_map<std::string, struct OHLCGroup> m_ohlc_data_;

	static void __setOHLCData(struct OHLCData &dt, uint64_t price,
				  uint64_t prec, uint64_t ts, uint64_t tsec);
	inline void __setOHLCGroup(const std::string &symbol, uint64_t price,
				   uint64_t prec, uint64_t ts);
	inline void setLastPrice(const std::string &symbol, const std::string &price,
				 uint64_t ts = 0);
	inline void delLastPrice(const std::string &symbol);

	inline void __addPriceUpdateCbBatch(const std::vector<std::string> &symbols,
					   PriceUpdateCb_t cb, void *udata = nullptr);
	inline void addPriceUpdateCbBatch(const std::vector<std::string> &symbols,
					   PriceUpdateCb_t cb, void *udata = nullptr);
	inline void __addPriceUpdateCbBatch(const std::vector<std::string> &symbols,
					   std::vector<PriceUpdateCb_t> cbs,
					   std::vector<void *> udatas);
	inline void addPriceUpdateCbBatch(const std::vector<std::string> &symbols,
					   std::vector<PriceUpdateCb_t> cbs,
					   std::vector<void *> udatas);
	inline void __delPriceUpdateCbBatch(const std::vector<std::string> &symbols);
	inline void delPriceUpdateCbBatch(const std::vector<std::string> &symbols);
	inline void __addPriceUpdateCb(const std::string &symbol, PriceUpdateCb_t cb,
				     void *udata = nullptr);
	inline void addPriceUpdateCb(const std::string &symbol, PriceUpdateCb_t cb,
				     void *udata = nullptr);
	inline void __delPriceUpdateCb(const std::string &symbol);
	inline void delPriceUpdateCb(const std::string &symbol);
	inline void getLastPriceNoListen(const std::string &symbol,
					 std::function<void(const std::string &)> cb);

protected:
	std::shared_ptr<Websocket> ws_ = nullptr;
	void invokePriceUpdateCb(const ExcPriceUpdate &up);

	virtual void __listenPriceUpdate(const std::string &symbol) = 0;
	virtual void __unlistenPriceUpdate(const std::string &symbol) = 0;
	virtual void __listenPriceUpdateBatch(const std::vector<std::string> &symbols);
	virtual void __unlistenPriceUpdateBatch(const std::vector<std::string> &symbols);

public:
	ExchangeFoundation(void);
	virtual ~ExchangeFoundation(void);

	static std::string formatPrice(uint64_t price, uint64_t precision);

	void listenPriceUpdate(const std::string &symbol,
				       PriceUpdateCb_t cb, void *udata);
	void unlistenPriceUpdate(const std::string &symbol);

	void listenPriceUpdateBatch(const std::vector<std::string> &symbols,
				    PriceUpdateCb_t cb, void *udata);
	void listenPriceUpdateBatch(const std::vector<std::string> &symbols,
				    std::vector<PriceUpdateCb_t> cbs,
				    std::vector<void *> udatas);
	void unlistenPriceUpdateBatch(const std::vector<std::string> &symbols);

	std::string getLastPrice(const std::string &symbol,
				 std::function<void(const std::string &)> cb = nullptr);

	void setWebsocket(std::shared_ptr<Websocket> ws);
	void dumpOHLCData(const std::string &symbol);

	virtual void start(void) = 0;
	virtual void close(void) = 0;
};

} /* namespace exc */
} /* namespace wbx */

#endif /* #ifndef EXC__EXCHANGE_FOUNDATION__HPP */
