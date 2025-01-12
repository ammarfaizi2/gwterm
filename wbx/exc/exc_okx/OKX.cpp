// SPDX-License-Identifier: GPL-2.0-only

#include <wbx/exc/exc_okx/OKX.hpp>
#include <wbx/nlohmann/json.hpp>
#include <string>
#include <exception>
#include <cstdio>

using json = nlohmann::json;

namespace wbx {
namespace exc {
namespace exc_OKX {

inline void OKX::handlePubWsChan(void *a)
{
	json &j = *static_cast<json *>(a);
	const std::string &chan = j["arg"]["channel"];

	if (chan == "mark-price")
		handlePubWsChanMarkPrice(a);
	else if (chan == "tickers")
		handlePubWsChanTickers(a);
}

inline void OKX::handlePubWsChanMarkPrice(void *a)
{
	json &j = *static_cast<json *>(a);

	if (!j["data"].is_array())
		return;

	for (const auto &d : j["data"]) {
		if (!d["instId"].is_string() || !d["markPx"].is_string())
			continue;

		struct ExcPriceUpdate pu;

		pu.symbol = d["instId"];
		pu.price = d["markPx"];
		pu.ts = std::stoul(d["ts"].get<std::string>());
		invokePriceUpdateCb(pu);
	}
}

inline void OKX::handlePubWsChanTickers(void *a)
{
	json &j = *static_cast<json *>(a);

	if (!j["data"].is_array())
		return;

	for (const auto &d : j["data"]) {
		if (!d["instId"].is_string() || !d["last"].is_string())
			continue;

		struct ExcPriceUpdate pu;

		pu.symbol = d["instId"];
		pu.price = d["last"];
		pu.ts = std::stoul(d["ts"].get<std::string>());
		invokePriceUpdateCb(pu);
	}
}

inline void OKX::handlePubWsOnWsConnect(void)
{
	ws_pub_started_ = true;
}

inline void OKX::handlePubWsOnWsWrite(size_t len)
{
	wss_pub_->read();
	(void)len;
}

inline size_t OKX::handlePubWsOnWsRead(const char *data, size_t len)
{
	try {
		std::string str(data, len);
		json j = json::parse(str);
		handlePubWsChan(&j);
		wss_pub_->read();
	} catch (const std::exception &e) {
	}

	return len;
}

inline void OKX::handlePubWsOnWsClose(void)
{
}

inline void OKX::startPubWs(void)
{
	if (wss_pub_)
		return;

	wss_pub_ = ws_->createSession("wspri.okx.com", 8443, "/ws/v5/ipublic");
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

inline void OKX::startPriWs(void)
{
	(void)wss_pri_;
}

void OKX::__listenPriceUpdate(const std::string &symbol)
{
	__listenPriceUpdateBatch({symbol});
}

void OKX::__unlistenPriceUpdate(const std::string &symbol)
{
	__unlistenPriceUpdateBatch({symbol});
}

void OKX::__listenPriceUpdateBatch(const std::vector<std::string> &symbols)
{
	json j;

	j["op"] = "subscribe";
	j["args"] = json::array();
	for (const auto &s : symbols) {
		json sub;
		sub["channel"] = "tickers";
		sub["instId"] = s;
		j["args"].push_back(sub);
	}

	wss_pub_->write(j.dump());
}

void OKX::__unlistenPriceUpdateBatch(const std::vector<std::string> &symbols)
{
	json j;

	j["op"] = "unsubscribe";
	j["args"] = json::array();
	for (const auto &s : symbols) {
		json sub;
		sub["channel"] = "tickers";
		sub["instId"] = s;
		j["args"].push_back(sub);
	}

	wss_pub_->write(j.dump());
}

void OKX::start(void)
{
	startPubWs();
	startPriWs();
}

void OKX::close(void)
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

OKX::OKX(void) = default;
OKX::~OKX(void) = default;

} /* namespace exc_OKX */
} /* namespace exc */
} /* namespace wbx */
