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

OKX::OKX(void) = default;

OKX::~OKX(void) = default;

void OKX::listenPriceUpdateBatch(const std::vector<std::string> &symbols,
				 PriceUpdateCb_t cb, void *udata)
{
	if (!ws_started_)
		throw std::runtime_error("WebSocket has not been started");

	json j;

	j["op"] = "subscribe";
	j["args"] = json::array();
	for (const auto &s : symbols) {
		j["args"].push_back({
			{"channel", "tickers"},
			{"instId", s}
		});
		addPriceUpdateCb(s, cb, udata);
	}

	std::string str = j.dump();
	wss_->write(str.c_str(), str.size());
}

void OKX::listenPriceUpdateBatch(const std::vector<std::string> &symbols,
				 std::vector<PriceUpdateCb_t> cbs,
				 std::vector<void *> udatas)
{
	if (!ws_started_)
		throw std::runtime_error("WebSocket has not been started");

	json j;

	j["op"] = "subscribe";
	j["args"] = json::array();
	for (size_t i = 0; i < symbols.size(); i++) {
		j["args"].push_back({
			{"channel", "tickers"},
			{"instId", symbols[i]}
		});
		addPriceUpdateCb(symbols[i], cbs[i], udatas[i]);
	}

	std::string str = j.dump();
	wss_->write(str.c_str(), str.size());
}

void OKX::unlistenPriceUpdateBatch(const std::vector<std::string> &symbols)
{
	if (!ws_started_)
		throw std::runtime_error("WebSocket has not been started");

	json j;

	j["op"] = "unsubscribe";
	j["args"] = json::array();
	for (const auto &s : symbols) {
		j["args"].push_back({
			{"channel", "tickers"},
			{"instId", s}
		});
		delPriceUpdateCb(s);
	}

	wss_->write(j.dump().c_str(), j.dump().size());
}

void OKX::listenPriceUpdate(const std::string &symbol, PriceUpdateCb_t cb,
			    void *udata)
{
	if (!ws_started_)
		throw std::runtime_error("WebSocket has not been started");

	return listenPriceUpdateBatch({symbol}, {cb}, {udata});
}

void OKX::unlistenPriceUpdate(const std::string &symbol)
{
	if (!ws_started_)
		throw std::runtime_error("WebSocket has not been started");

	return unlistenPriceUpdateBatch({symbol});
}

void OKX::onWsConnect(void)
{
}

void OKX::onWsWrite(size_t len)
{
	wss_->read();
	(void)len;
}

size_t OKX::onWsRead(const char *data, size_t len)
{
	try {
		std::string str(data, len);
		json j = json::parse(str);
		handleWsChannel(&j);
		wss_->read();
	} catch (const std::exception &e) {
	}

	return len;
}

void OKX::onWsClose(void)
{
	ws_started_ = false;
}

void OKX::start(void)
{
	if (ws_started_)
		return;

	if (!ws_)
		throw std::runtime_error("WebSocket has not been initialized");

	wss_ = ws_->createSession("wspri.okx.com", 8443, "/ws/v5/ipublic");

	wss_->setOnConnect([this](WebsocketSession *ws_sess) {
		onWsConnect();
		(void)ws_sess;
	});

	wss_->setOnWrite([this](WebsocketSession *ws_sess, size_t len) {
		onWsWrite(len);
		(void)ws_sess;
	});

	wss_->setOnRead([this](WebsocketSession *ws_sess, const char *data, size_t len) {
		(void)ws_sess;
		return onWsRead(data, len);
	});

	wss_->setOnClose([this](WebsocketSession *ws_sess) {
		onWsClose();
		(void)ws_sess;
	});

	wss_->run();
	ws_started_ = true;
}

inline void OKX::handleWsChannel(void *a)
{
	json &j = *static_cast<json *>(a);
	const std::string &chan = j["arg"]["channel"];

	if (chan == "mark-price")
		handleChanMarkPrice(a);
	else if (chan == "tickers")
		handleChanTickers(a);
}

inline void OKX::handleChanMarkPrice(void *a)
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

inline void OKX::handleChanTickers(void *a)
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

} /* namespace exc_OKX */
} /* namespace exc */
} /* namespace wbx */
