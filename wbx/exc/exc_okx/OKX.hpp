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

protected:
	virtual void onWsConnect(void);
	virtual void onWsWrite(size_t len);
	virtual size_t onWsRead(const char *data, size_t len);
	virtual void onWsClose(void);

public:
	OKX(void);
	~OKX(void);

	virtual void listenPriceUpdate(const std::string &symbol, price_update_cb_t cb, void *udata);
	virtual void unlistenPriceUpdate(const std::string &symbol);
	virtual void start(void);
};

} /* namespace exc_OKX */
} /* namespace exc */
} /* namespace wbx */
