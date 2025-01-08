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

struct price_update {
	std::string	symbol;
	std::string	price;
	uint64_t	ts;
};

class ExchangeFoundation;

typedef std::function<void(ExchangeFoundation *ef, const price_update &up, void *udata)> price_update_cb_t;

struct price_update_cb_data {
	price_update_cb_t	cb;
	void			*udata;
};

struct PendingWrites {
	std::vector<std::string>	writes_;
};

class ExchangeFoundation {
private:
	std::mutex m_price_update_cbs_mtx_;
	std::unordered_map<std::string, price_update_cb_data> m_price_update_cbs_;

	std::mutex m_last_prices_mtx_;
	std::unordered_map<std::string, uint64_t> m_last_prices_;
	std::unordered_map<std::string, uint64_t> m_precisions_;

protected:
	std::shared_ptr<Websocket> ws_ = nullptr;
	WebsocketSession *wss_ = nullptr;

	void addPriceUpdateCb(const std::string &symbol, price_update_cb_t cb, void *udata = nullptr);
	void delPriceUpdateCb(const std::string &symbol);
	void invokePriceUpdateCb(const price_update &up);
	void setLastPrice(const std::string &symbol, const std::string &price);
	void delLastPrice(const std::string &symbol);
	virtual void onWsConnect(void) = 0;
	virtual void onWsWrite(size_t len) = 0;
	virtual size_t onWsRead(const char *data, size_t len) = 0;
	virtual void onWsClose(void) = 0;

public:
	ExchangeFoundation(void);
	~ExchangeFoundation(void);

	virtual void listenPriceUpdate(const std::string &symbol, price_update_cb_t cb, void *udata) = 0;
	virtual void unlistenPriceUpdate(const std::string &symbol) = 0;
	virtual void start(void) = 0;
	virtual std::string getLastPrice(const std::string &symbol);

	void setWebsocket(std::shared_ptr<Websocket> ws);
};

} /* namespace exc */
} /* namespace wbx */

#endif /* #ifndef EXC__EXCHANGE_FOUNDATION__HPP */
