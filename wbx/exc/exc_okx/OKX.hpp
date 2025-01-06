// SPDX-License-Identifier: GPL-2.0-only

#include <string>
#include <functional>
#include <wbx/exc/ExchangeFoundation.hpp>

namespace wbx {
namespace exc {
namespace exc_OKX {

class OKX: public exc::ExchangeFoundation {
public:
	OKX(void);
	~OKX(void);

	virtual void listenPriceUpdate(const std::string &symbol, price_update_cb_t cb, void *udata);
	virtual void unlistenPriceUpdate(const std::string &symbol);
};

} /* namespace exc_OKX */
} /* namespace exc */
} /* namespace wbx */
