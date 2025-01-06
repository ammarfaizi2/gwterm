// SPDX-License-Identifier: GPL-2.0-only

#ifndef EXC__EXCHANGE_FOUNDATION__HPP
#define EXC__EXCHANGE_FOUNDATION__HPP

#include <string>
#include <mutex>
#include <functional>
#include <unordered_map>

namespace wbx {
namespace exc {

struct price_update {
	std::string	symbol;
	uint64_t	ts;
	double		price;
};

class ExchangeFoundation;

typedef std::function<void(ExchangeFoundation *ef, const price_update &up, void *udata)> price_update_cb_t;

struct price_update_cb_data {
	price_update_cb_t	cb;
	void			*udata;
};

class ExchangeFoundation {
private:
	std::mutex m_price_update_cbs_mtx_;
	std::unordered_map<std::string, price_update_cb_data> m_price_update_cbs_;

	std::mutex m_last_prices_mtx_;
	std::unordered_map<std::string, double> m_last_prices_;

protected:
	void addPriceUpdateCb(const std::string &symbol, price_update_cb_t cb, void *udata = nullptr);
	void delPriceUpdateCb(const std::string &symbol);
	void invokePriceUpdateCb(const price_update &up);
	void setLastPrice(const std::string &symbol, double price);
	void delLastPrice(const std::string &symbol);

public:
	ExchangeFoundation(void);
	~ExchangeFoundation(void);

	virtual void listenPriceUpdate(const std::string &symbol, price_update_cb_t cb, void *udata) = 0;
	virtual void unlistenPriceUpdate(const std::string &symbol) = 0;
	virtual double getLastPrice(const std::string &symbol);
};

} /* namespace exc */
} /* namespace wbx */

#endif /* #ifndef EXC__EXCHANGE_FOUNDATION__HPP */
