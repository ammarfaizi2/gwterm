// SPDX-License-Identifier: GPL-2.0-only

#ifdef EXC_USE_WEBSOCKET_IMPL
#ifndef EXC__WEBSOCKETIMPL__HPP
#define EXC__WEBSOCKETIMPL__HPP

#include <boost/asio.hpp>
#include <boost/beast.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/websocket.hpp>
#include <boost/beast/websocket/ssl.hpp>
#include <boost/beast/ssl.hpp>
#include <boost/asio/strand.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/ssl.hpp>

#include <cstdlib>
#include <functional>
#include <iostream>
#include <memory>
#include <string>
#include <atomic>
#include <queue>

namespace wbx {
namespace exc {

namespace beast = boost::beast;         // from <boost/beast.hpp>
namespace http = beast::http;           // from <boost/beast/http.hpp>
namespace websocket = beast::websocket; // from <boost/beast/websocket.hpp>
namespace net = boost::asio;            // from <boost/asio.hpp>
namespace ssl = boost::asio::ssl;       // from <boost/asio/ssl.hpp>
using tcp = boost::asio::ip::tcp;       // from <boost/asio/ip/tcp.hpp>

class WebsocketImplSession;

typedef std::function<void(WebsocketImplSession *ws_sess, void *udata)> WsImplOnConnect_t;
typedef std::function<size_t(WebsocketImplSession *ws_sess, const char *data, size_t len, void *udata)> WsImplOnRead_t;
typedef std::function<void(WebsocketImplSession *ws_sess, size_t len, void *udata)> WsImplOnWrite_t;
typedef std::function<void(WebsocketImplSession *ws_sess, void *udata)> WsImplOnClose_t;
typedef std::function<void(WebsocketImplSession *ws_sess, int code, const char *msg, void *udata)> WsImplOnConnErr_t;

struct write_buf {
	void	*data_;
	size_t	len_;

	inline write_buf(void) noexcept:
		data_(nullptr),
		len_(0)
	{
	}

	inline ~write_buf(void) noexcept
	{
		if (data_)
			free(data_);
	}

	inline write_buf(write_buf &&other) noexcept:
		data_(other.data_),
		len_(other.len_)
	{
		other.data_ = nullptr;
		other.len_ = 0;
	}

	inline write_buf &operator=(write_buf &&other) noexcept
	{
		if (this != &other) {
			if (data_)
				free(data_);
			data_ = other.data_;
			len_ = other.len_;
			other.data_ = nullptr;
			other.len_ = 0;
		}
		return *this;
	}

	inline bool set(const void *data, size_t len) noexcept
	{
		void *new_data = malloc(len);
		if (!new_data)
			return false;

		memcpy(new_data, data, len);
		data_ = new_data;
		len_ = len;
		return true;
	}

	inline const void *data(void) const noexcept { return data_; }
	inline size_t len(void) const noexcept { return len_; }
};

class WebsocketImplSession: public std::enable_shared_from_this<WebsocketImplSession> {
private:
	websocket::stream<beast::ssl_stream<beast::tcp_stream>> ws_;
	tcp::resolver		resolver_;
	beast::flat_buffer	buffer_;
	std::string		user_agent_;
	std::string		uri_;
	std::string		host_;
	uint16_t		port_;

	void			*udata_ = nullptr;
	WsImplOnConnect_t	onConnect_ = nullptr;
	WsImplOnRead_t		onRead_ = nullptr;
	WsImplOnWrite_t		onWrite_ = nullptr;
	WsImplOnClose_t		onClose_ = nullptr;
	WsImplOnConnErr_t	onConnErr_ = nullptr;
	std::atomic<int64_t>	nr_read_after_;

	std::mutex			wq_mtx_;
	std::queue<struct write_buf>	write_queue_;

	bool popNrRead(void);
	inline void invokeOnConnErr(beast::error_code &ec);

public:
	explicit WebsocketImplSession(net::io_context &ioc, ssl::context &ctx);
	~WebsocketImplSession(void);

	inline void setUserAgent(const std::string &userAgent) { user_agent_ = userAgent; }
	inline void setUri(const std::string &uri) { uri_ = uri; }
	inline void setHost(const std::string &host) { host_ = host; }
	inline void setPort(uint16_t port) { port_ = port; }
	inline void setUData(void *udata) { udata_ = udata; }
	inline void setOnConnect(WsImplOnConnect_t onConnect) { onConnect_ = onConnect; }
	inline void setOnRead(WsImplOnRead_t onRead) { onRead_ = onRead; }
	inline void setOnWrite(WsImplOnWrite_t onWrite) { onWrite_ = onWrite; }
	inline void setOnClose(WsImplOnClose_t onClose) { onClose_ = onClose; }
	inline void setOnConnErr(WsImplOnConnErr_t onConnErr) { onConnErr_ = onConnErr; }

	void run(void);
	void onResolve(beast::error_code ec, tcp::resolver::results_type results);
	void onConnect(beast::error_code ec, tcp::resolver::results_type::endpoint_type ep);
	void onSslHandshake(beast::error_code ec);
	void onHandshake(beast::error_code ec);
	void onWrite(beast::error_code ec, std::size_t bytes_transferred);
	void onRead(beast::error_code ec, std::size_t bytes_transferred);
	void onClose(beast::error_code ec);

	void write(const void *data, size_t len);
	void read(void);
	inline void readAfter(void) { nr_read_after_.fetch_add(1); }
};

class WebsocketImpl {
private:
	net::io_context		io_ctx_;
	ssl::context		ssl_ctx_;

public:
	WebsocketImpl(void);
	~WebsocketImpl(void);

	std::shared_ptr<WebsocketImplSession> createSession(void);

	inline net::io_context &getIOCtx(void) { return io_ctx_; }
	inline ssl::context &getSSLCtx(void) { return ssl_ctx_; }
	inline void run(void) { io_ctx_.run(); }
};

} /* namespace exc */
} /* namespace wbx */

#endif /* #ifndef EXC__WEBSOCKETIMPL__HPP */
#endif /* #ifdef EXC_USE_WEBSOCKET_IMPL */
