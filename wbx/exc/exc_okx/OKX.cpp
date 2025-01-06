// SPDX-License-Identifier: GPL-2.0-only

#include <wbx/exc/exc_okx/OKX.hpp>

namespace wbx {
namespace exc {
namespace exc_OKX {

OKX::OKX(void) = default;

OKX::~OKX(void) = default;

void OKX::listenPriceUpdate(const std::string &symbol,
			    price_update_cb_t cb, void *udata)
{
	addPriceUpdateCb(symbol, cb, udata);
}

void OKX::unlistenPriceUpdate(const std::string &symbol)
{
	delPriceUpdateCb(symbol);
}

} /* namespace exc_OKX */
} /* namespace exc */
} /* namespace wbx */
