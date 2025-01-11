// SPDX-License-Identifier: GPL-2.0-only

#include <string>
#include <functional>
#include <wbx/exc/ExchangeFoundation.hpp>

namespace wbx {
namespace exc {
namespace exc_OKX {

class OKX: public exc::ExchangeFoundation {
private:
	bool ws_started_ = false;

	inline void handleWsChannel(void *a);
	inline void handleChanMarkPrice(void *a);
	inline void handleChanTickers(void *a);

protected:
	virtual void onWsConnect(void);
	virtual void onWsWrite(size_t len);
	virtual size_t onWsRead(const char *data, size_t len);
	virtual void onWsClose(void);

public:
	OKX(void);
	~OKX(void);

	virtual void listenPriceUpdate(const std::string &symbol, PriceUpdateCb_t cb, void *udata);
	virtual void unlistenPriceUpdate(const std::string &symbol);
	virtual void listenPriceUpdateBatch(const std::vector<std::string> &symbols, PriceUpdateCb_t cb, void *udata);
	virtual void listenPriceUpdateBatch(const std::vector<std::string> &symbols, std::vector<PriceUpdateCb_t> cbs, std::vector<void *> udatas);
	virtual void unlistenPriceUpdateBatch(const std::vector<std::string> &symbols);
	virtual void start(void);
};

} /* namespace exc_OKX */
} /* namespace exc */
} /* namespace wbx */
