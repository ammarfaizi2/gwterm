// SPDX-License-Identifier: GPL-2.0-only

#include <string>
#include <functional>
#include <wbx/exc/ExchangeFoundation.hpp>

namespace wbx {
namespace exc {
namespace exc_OKX {

class OKX: public exc::ExchangeFoundation {
private:
	bool ws_pub_started_ = false;
	bool ws_pri_started_ = false;
	WebsocketSession *wss_pub_ = nullptr;
	WebsocketSession *wss_pri_ = nullptr;

	inline void handlePubWsChan(void *a);
	inline void handlePubWsChanMarkPrice(void *a);
	inline void handlePubWsChanTickers(void *a);

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
	OKX(void);
	virtual ~OKX(void);
	virtual void start(void) override;
};

} /* namespace exc_OKX */
} /* namespace exc */
} /* namespace wbx */
