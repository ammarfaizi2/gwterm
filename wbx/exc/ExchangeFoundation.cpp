// SPDX-License-Identifier: GPL-2.0-only

#include <wbx/exc/ExchangeFoundation.hpp>
#include <cstdio>

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
	printf("addPriceUpdateCb: %s\n", symbol.c_str());
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

void ExchangeFoundation::setLastPrice(const std::string &symbol,
				      const std::string &price)
{
	uint64_t precision;

	auto dot = price.find('.');
	if (dot == std::string::npos)
		precision = 0;
	else
		precision = price.size() - dot - 1;

	std::lock_guard<std::mutex> lock(m_last_prices_mtx_);
	m_last_prices_[symbol] = std::stoull(price);
	m_precisions_[symbol] = precision;
}

void ExchangeFoundation::delLastPrice(const std::string &symbol)
{
	std::lock_guard<std::mutex> lock(m_last_prices_mtx_);
	m_last_prices_.erase(symbol);
}

std::string ExchangeFoundation::getLastPrice(const std::string &symbol)
{
	std::lock_guard<std::mutex> lock(m_last_prices_mtx_);

	auto it = m_last_prices_.find(symbol);
	if (it != m_last_prices_.end()) {
		uint64_t precision = m_precisions_[symbol];
		char buf[32];

		snprintf(buf, sizeof(buf), "%.*f", (int)precision, it->second / 1e8);
		return buf;
	}

	return "";
}

void ExchangeFoundation::setWebsocket(std::shared_ptr<Websocket> ws)
{
	ws_ = ws;
}

} /* namespace exc */
} /* namespace wbx */
