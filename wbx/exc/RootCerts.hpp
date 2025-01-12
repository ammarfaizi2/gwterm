// SPDX-License-Identifier: GPL-2.0-only
#ifndef EXC__ROOTCERTS__HPP
#define EXC__ROOTCERTS__HPP

#include <boost/beast/ssl.hpp>

void load_root_certificates(boost::asio::ssl::context &ctx, boost::system::error_code &ec);
void load_root_certificates(boost::asio::ssl::context &ctx);

#endif /* #ifndef EXC__ROOTCERTS__HPP */
