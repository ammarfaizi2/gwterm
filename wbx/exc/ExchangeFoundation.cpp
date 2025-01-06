// SPDX-License-Identifier: GPL-2.0-only

#include <wbx/exc/ExchangeFoundation.hpp>

namespace wbx {
namespace exc {

ExchangeFoundation::ExchangeFoundation(void) = default;

ExchangeFoundation::~ExchangeFoundation(void) = default;

void ExchangeFoundation::addPriceUpdateCb(const std::string &symbol,
					  price_update_cb_t cb,
					  void *udata)
{
	std::lock_guard<std::mutex> lock(m_price_update_cbs_mtx_);
	m_price_update_cbs_[symbol] = {cb, udata};
}

void ExchangeFoundation::delPriceUpdateCb(const std::string &symbol)
{
	std::lock_guard<std::mutex> lock(m_price_update_cbs_mtx_);
	m_price_update_cbs_.erase(symbol);
}

void ExchangeFoundation::invokePriceUpdateCb(const price_update &up)
{
	std::lock_guard<std::mutex> lock(m_price_update_cbs_mtx_);

	auto it = m_price_update_cbs_.find(up.symbol);
	if (it != m_price_update_cbs_.end()) {
		setLastPrice(up.symbol, up.price);

		auto &d = it->second;
		d.cb(this, up, d.udata);
	}
}

void ExchangeFoundation::setLastPrice(const std::string &symbol, double price)
{
	std::lock_guard<std::mutex> lock(m_last_prices_mtx_);
	m_last_prices_[symbol] = price;
}

void ExchangeFoundation::delLastPrice(const std::string &symbol)
{
	std::lock_guard<std::mutex> lock(m_last_prices_mtx_);
	m_last_prices_.erase(symbol);
}

double ExchangeFoundation::getLastPrice(const std::string &symbol)
{
	std::lock_guard<std::mutex> lock(m_last_prices_mtx_);

	auto it = m_last_prices_.find(symbol);
	if (it != m_last_prices_.end())
		return it->second;

	return 0.0;
}

} /* namespace exc */
} /* namespace wbx */
