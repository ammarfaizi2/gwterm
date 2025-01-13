// SPDX-License-Identifier: GPL-2.0-only

#include <wbx/exc/exc_binance/Binance.hpp>
#include <wbx/nlohmann/json.hpp>
#include <string>
#include <exception>
#include <cstdio>

using json = nlohmann::json;

namespace wbx {
namespace exc {
namespace exc_Binance {

static std::string normalize_pair(const std::string &pair)
{
	/*
	 * Convert BTC-USDT to btcusdt
	 */
	std::string ret = pair;
	for (auto &c : ret)
		c = std::tolower((unsigned char)c);
	
	// Remove hyphen
	ret.erase(std::remove(ret.begin(), ret.end(), '-'), ret.end());
	return ret;
}

inline void Binance::handlePubWsChan(void *a)
{
	json &j = *static_cast<json *>(a);

	if (!j.contains("stream"))
		return;

	const std::string &chan = j["stream"];
	size_t pos = chan.find('@');
	if (pos == std::string::npos)
		return;

	std::string chan_name = chan.substr(pos + 1);
	if (chan_name == "aggTrade")
		handlePubWsChanAggTrade(a);
}

static std::string rtrim_trailing_zeroes(const std::string &s)
{
	size_t pos = s.find('.');
	if (pos == std::string::npos)
		return s;

	size_t end = s.size() - 1;
	while (s[end] == '0')
		end--;

	if (s[end] == '.')
		end--;

	return s.substr(0, end + 1);
}

inline void Binance::handlePubWsChanAggTrade(void *a)
{
	json &j = *static_cast<json *>(a);
	if (!j.contains("data"))
		return;

	const json &d = j["data"];
	std::string sym = d["s"];
	std::string price = d["p"];
	uint64_t ts = d["T"].get<uint64_t>();

	auto it = normalized_pairs_.find(normalize_pair(sym));
	if (it == normalized_pairs_.end())
		return;

	struct ExcPriceUpdate pu;
	pu.symbol = it->second;
	pu.price = rtrim_trailing_zeroes(price);
	pu.ts = ts;
	invokePriceUpdateCb(pu);
}

inline void Binance::handlePubWsOnWsConnect(void)
{
	ws_pub_started_ = true;
}

inline void Binance::handlePubWsOnWsWrite(size_t len)
{
	wss_pub_->read();
	(void)len;
}

inline size_t Binance::handlePubWsOnWsRead(const char *data, size_t len)
{
	try {
		std::string str(data, len);
		json j = json::parse(str);
		handlePubWsChan(&j);
		wss_pub_->read();
	} catch (const std::exception &e) {
		printf("Exception: %s\n", e.what());
	}

	return len;
}

inline void Binance::handlePubWsOnWsClose(void)
{
}

inline void Binance::startPubWs(void)
{
	if (wss_pub_)
		return;

	wss_pub_ = ws_->createSession("stream.binance.com", 443, "/stream");
	wss_pub_->setOnConnect([this](WebsocketSession *ws_sess) {
		handlePubWsOnWsConnect();
		(void)ws_sess;
	});

	wss_pub_->setOnWrite([this](WebsocketSession *ws_sess, size_t len) {
		handlePubWsOnWsWrite(len);
		(void)ws_sess;
	});

	wss_pub_->setOnRead([this](WebsocketSession *ws_sess, const char *data, size_t len) {
		(void)ws_sess;
		return handlePubWsOnWsRead(data, len);
	});

	wss_pub_->setOnClose([this](WebsocketSession *ws_sess) {
		handlePubWsOnWsClose();
		(void)ws_sess;
	});

	wss_pub_->run();
}

inline void Binance::startPriWs(void)
{
	(void)wss_pri_;
}

void Binance::__listenPriceUpdate(const std::string &symbol)
{
	__listenPriceUpdateBatch({symbol});
}

void Binance::__unlistenPriceUpdate(const std::string &symbol)
{
	__unlistenPriceUpdateBatch({symbol});
}

void Binance::__listenPriceUpdateBatch(const std::vector<std::string> &symbols)
{
	json j;

	j["method"] = "SUBSCRIBE";
	j["params"] = json::array();
	for (const auto &s : symbols) {
		std::string p = normalize_pair(s);
		normalized_pairs_[p] = s;
		j["params"].push_back(p + "@aggTrade");
	}
	j["id"] = id_++;

	wss_pub_->write(j.dump());
}

void Binance::__unlistenPriceUpdateBatch(const std::vector<std::string> &symbols)
{
	// json j;

	// j["op"] = "unsubscribe";
	// j["args"] = json::array();
	// for (const auto &s : symbols) {
	// 	json sub;
	// 	sub["channel"] = "tickers";
	// 	sub["instId"] = s;
	// 	j["args"].push_back(sub);
	// }

	// wss_pub_->write(j.dump());
}

void Binance::start(void)
{
	startPubWs();
	startPriWs();
}

void Binance::close(void)
{
	if (wss_pub_) {
		wss_pub_->close();
		wss_pub_ = nullptr;
	}

	if (wss_pri_) {
		wss_pri_->close();
		wss_pri_ = nullptr;
	}
}

Binance::Binance(void) = default;
Binance::~Binance(void) = default;

} /* namespace exc_Binance */
} /* namespace exc */
} /* namespace wbx */
