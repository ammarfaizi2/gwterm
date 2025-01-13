// SPDX-License-Identifier: GPL-2.0-only

#ifndef EXC__EXCHANGE__EXC_BINANCE__BINANCE__HPP
#define EXC__EXCHANGE__EXC_BINANCE__BINANCE__HPP

#include <string>
#include <functional>
#include <unordered_map>
#include <wbx/exc/ExchangeFoundation.hpp>

namespace wbx {
namespace exc {
namespace exc_Binance {

class Binance: public exc::ExchangeFoundation {
private:
	bool ws_pub_started_ = false;
	bool ws_pri_started_ = false;
	WebsocketSession *wss_pub_ = nullptr;
	WebsocketSession *wss_pri_ = nullptr;
	uint64_t id_ = 1;

	std::unordered_map<std::string, std::string> normalized_pairs_;

	inline void handlePubWsChan(void *a);
	inline void handlePubWsChanAggTrade(void *a);

	inline void handlePubWsOnWsConnect(void);
	inline void handlePubWsOnWsWrite(size_t len);
	inline size_t handlePubWsOnWsRead(const char *data, size_t len);
	inline void handlePubWsOnWsClose(void);

	inline void startPubWs(void);
	inline void startPriWs(void);

protected:
	virtual void __listenPriceUpdate(const std::string &symbol) override;
	virtual void __unlistenPriceUpdate(const std::string &symbol) override;
	virtual void __listenPriceUpdateBatch(const std::vector<std::string> &symbols) override;
	virtual void __unlistenPriceUpdateBatch(const std::vector<std::string> &symbols) override;

public:
	Binance(std::shared_ptr<Websocket> ws);
	virtual ~Binance(void);
	virtual void start(void) override;
	virtual void close(void) override;
};

} /* namespace exc_OKX */
} /* namespace exc */
} /* namespace wbx */

#endif /* #ifndef EXC__EXCHANGE__EXC_BINANCE__BINANCE__HPP */
