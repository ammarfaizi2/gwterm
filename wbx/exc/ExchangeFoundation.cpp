// SPDX-License-Identifier: GPL-2.0-only

#include <wbx/exc/ExchangeFoundation.hpp>
#include <cstdio>
#include <cstring>
#include <cmath>

namespace wbx {
namespace exc {

ExchangeFoundation::ExchangeFoundation(void) = default;

ExchangeFoundation::~ExchangeFoundation(void) = default;

void ExchangeFoundation::addPriceUpdateCb(const std::string &symbol,
					  PriceUpdateCb_t cb,
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

void ExchangeFoundation::invokePriceUpdateCb(const ExcPriceUpdate &up)
{
	std::lock_guard<std::mutex> lock(m_price_update_cbs_mtx_);

	auto it = m_price_update_cbs_.find(up.symbol);
	if (it != m_price_update_cbs_.end()) {
		setLastPrice(up.symbol, up.price);

		auto &d = it->second;
		d.cb(this, up, d.udata);
	}
}

void ExchangeFoundation::dumpOHLCData(const std::string &symbol)
{
	static const char tred[] = "\033[31m";
	static const char tgreen[] = "\033[32m";
	const struct OHLCGroup &og = m_ohlc_data_[symbol];
	const struct OHLCPrice &p = og.ohlc_1m_.prices.back();

	if (p.curr == p.prev)
		return;

	std::string open = formatPrice(p.open, p.prec);
	std::string high = formatPrice(p.high, p.prec);
	std::string low = formatPrice(p.low, p.prec);
	std::string close = formatPrice(p.close, p.prec);
	std::string curr = formatPrice(p.curr, p.prec);
	std::string prev = formatPrice(p.prev, p.prec);

	if (p.close > p.open)
		printf("%s", tgreen);
	else if (p.close < p.open)
		printf("%s", tred);

	printf("%s\033[0m | tso: %llu, cts: %llu, tsc: %llu, O: %s, H: %s, L: %s, C: %s, curr: %s, prev: %s",
	       symbol.c_str(), (unsigned long long)p.ts_open, (unsigned long long)p.ts_last,
	       (unsigned long long)p.ts_close, open.c_str(), high.c_str(), low.c_str(),
	       close.c_str(), curr.c_str(), prev.c_str());

	if (p.close != p.open) {
		double diff = ((double)p.close - (double)p.open) / p.prec;
		double perc = (diff / (double)p.open) * 100.0;
		printf(", diff: %.2f, perc: %.2f%%", diff, perc);
	}

	printf("\n");
}

// static
void ExchangeFoundation::__setOHLCData(struct OHLCData &dt, uint64_t price,
				       uint64_t prec, uint64_t ts, uint64_t tsec)
{
	if (dt.nr_samples == 0) {
		uint64_t ts_close;

		ts_close = ts / 1000;
		ts_close = ts_close - (ts_close % tsec) + tsec;
		ts_close *= 1000;

		dt.prices.push_back({ts, ts, ts_close, price, price, price, price, price, price, prec});
		dt.nr_samples++;
		return;
	}
	
	auto &p = dt.prices.back();

	if (p.ts_close < ts) {
		uint64_t ts_close, ts_open;

		ts_close = ts / 1000;
		ts_close = ts_close - (ts_close % tsec) + tsec;
		ts_close *= 1000;
		ts_open = p.ts_close;

		dt.prices.push_back({ts, ts_open, ts_close, price, price, price, price, price, price, prec});
		dt.nr_samples++;
	} else {
		if (p.prec < prec) {
			uint64_t mul, prec_diff;

			prec_diff = prec - p.prec;
			p.prec = prec;

			mul = std::pow(10, prec_diff);
			p.open *= mul;
			p.high *= mul;
			p.low *= mul;
			p.close *= mul;
			p.curr *= mul;
			p.prev *= mul;
		} else {
			uint64_t mul, prec_diff;

			prec_diff = p.prec - prec;
			mul = std::pow(10, prec_diff);
			price *= mul;
		}

		p.close = price;
		if (price > p.high)
			p.high = price;
		if (price < p.low)
			p.low = price;

		p.prev = p.curr;
		p.curr = price;
		p.ts_last = ts;
	}
}

// Must hold m_last_prices_mtx_ lock.
void ExchangeFoundation::__setOHLCGroup(const std::string &symbol, uint64_t price,
					uint64_t prec, uint64_t ts)
{
	struct OHLCGroup &og = m_ohlc_data_[symbol];

	__setOHLCData(og.ohlc_1s_, price, prec, ts, 1);
	__setOHLCData(og.ohlc_1m_, price, prec, ts, 60);
	__setOHLCData(og.ohlc_5m_, price, prec, ts, 300);
	__setOHLCData(og.ohlc_15m_, price, prec, ts, 900);
	__setOHLCData(og.ohlc_30m_, price, prec, ts, 1800);
	__setOHLCData(og.ohlc_1h_, price, prec, ts, 3600);
	__setOHLCData(og.ohlc_4h_, price, prec, ts, 14400);
}

void ExchangeFoundation::setLastPrice(const std::string &symbol,
				      const std::string &price_c,
				      uint64_t ts)
{
	std::lock_guard<std::mutex> lock(m_last_prices_mtx_);
	auto it = m_precisions_.find(symbol);
	std::string price = price_c;
	uint64_t cur_price;
	uint64_t cur_prec;

	auto dot = price.find('.');
	if (dot == std::string::npos) {
		cur_prec = 0;
	} else {
		cur_prec = price.size() - dot - 1;
		price.erase(dot, 1);
	}

	if (it != m_precisions_.end()) {
		uint64_t old_prec = it->second;

		if (cur_prec < old_prec) {
			uint64_t i, prec_diff = old_prec - cur_prec;
			for (i = 0; i < prec_diff; i++)
				price += '0';

			cur_prec = old_prec;
		} else if (cur_prec > old_prec) {
			m_precisions_[symbol] = cur_prec;
		}
	} else {
		m_precisions_[symbol] = cur_prec;
	}

	if (ts == 0) {
		ts = std::chrono::duration_cast<std::chrono::milliseconds>(
			std::chrono::system_clock::now().time_since_epoch()).count();
	}

	cur_price = std::stoull(price);
	m_last_prices_[symbol] = cur_price;
	__setOHLCGroup(symbol, cur_price, cur_prec, ts);
}

void ExchangeFoundation::delLastPrice(const std::string &symbol)
{
	std::lock_guard<std::mutex> lock(m_last_prices_mtx_);
	m_last_prices_.erase(symbol);
}

// static
std::string ExchangeFoundation::formatPrice(uint64_t price, uint64_t prec)
{
	char *dst, *src;
	size_t cp_len;
	char buf[128];
	size_t len;

	len = (size_t)snprintf(buf, sizeof(buf), "%llu", (unsigned long long)price);
	if (prec >= len) {
		dst = &buf[prec - len + 2];
		src = buf;
		cp_len = len + 1;
		memmove(dst, src, cp_len);
		buf[0] = '0';
		buf[1] = '.';
		memset(buf + 2, '0', prec - len);
	} else {
		dst = buf + len - prec + 1;
		src = buf + len - prec;
		cp_len = prec + 1;
		memmove(dst, src, cp_len);
		buf[len - prec] = '.';
	}

	return buf;
}

std::string ExchangeFoundation::getLastPrice(const std::string &symbol)
{
	uint64_t price, prec;

	{
		std::lock_guard<std::mutex> lock(m_last_prices_mtx_);
		auto it_price = m_last_prices_.find(symbol);
		auto it_prec = m_precisions_.find(symbol);
		if (it_price == m_last_prices_.end() || it_prec == m_precisions_.end())
			return "";

		price = it_price->second;
		prec = it_prec->second;
	}

	if (prec == 0)
		return std::to_string(price);

	return formatPrice(price, prec);
}

void ExchangeFoundation::setWebsocket(std::shared_ptr<Websocket> ws)
{
	ws_ = ws;
}

} /* namespace exc */
} /* namespace wbx */
