// SPDX-License-Identifier: GPL-2.0-only

#ifndef EXC__WEBSOCKET__HPP
#define EXC__WEBSOCKET__HPP

#include <thread>
#include <vector>
#include <memory>
#include <string>
#include <cstdint>
#include <functional>
#include <wbx/exc/WebsocketImpl.hpp>

/*
 * Boost is a very heavy header. This header hides the implementation of the
 * websocket client for fast compilation.
 */
namespace wbx {
namespace exc {

class WebsocketSession;
class Websocket;

typedef std::function<void(WebsocketSession *ws_sess)> WsOnConnect_t;
typedef std::function<size_t(WebsocketSession *ws_sess, const char *data, size_t len)> WsOnRead_t;
typedef std::function<void(WebsocketSession *ws_sess, size_t len)> WsOnWrite_t;
typedef std::function<void(WebsocketSession *ws_sess)> WsOnClose_t;

class WebsocketSession {
private:
#ifdef EXC_USE_WEBSOCKET_IMPL
	WebsocketImplSession			*ws_sess_ = nullptr;
	std::shared_ptr<WebsocketImplSession>	*ws_sess_shr_ = nullptr;
#else
	void					*ws_sess_ = nullptr;
	void					*ws_sess_shr_ = nullptr;
#endif
	Websocket	*ws_;

public:
	WebsocketSession(Websocket *ws, const std::string &host = "",
			 uint16_t port = 8443, const std::string &uri = "/");
	~WebsocketSession(void);

	void setHost(const std::string &host);
	void setPort(uint16_t port);
	void setUri(const std::string &uri);
	void setUserAgent(const std::string &userAgent);

	void setOnConnect(WsOnConnect_t onConnect);
	void setOnRead(WsOnRead_t onRead);
	void setOnWrite(WsOnWrite_t onWrite);
	void setOnClose(WsOnClose_t onClose);

	void write(const char *data, size_t len);
	inline void write(const std::string &data) { write(data.c_str(), data.size()); }
	void read(void);
	void run(void);
};

class Websocket {
private:
#ifdef EXC_USE_WEBSOCKET_IMPL
	WebsocketImpl	*ws_;
#else
	void		*ws_;
#endif
	std::vector<std::unique_ptr<WebsocketSession>> ws_sessions_;
	std::unique_ptr<std::thread> ws_thread_;

public:
	Websocket(void);
	~Websocket(void);

#ifdef EXC_USE_WEBSOCKET_IMPL
	inline net::io_context &getIOCtx(void) { return ws_->getIOCtx(); }
	inline ssl::context &getSSLCtx(void) { return ws_->getSSLCtx(); }
#endif

	void run(void);
	void bgRun(void);

	WebsocketSession *createSession(const std::string &host = "",
					uint16_t port = 8443,
					const std::string &uri = "/");
};

} /* namespace exc */
} /* namespace wbx */

#endif /* #ifndef EXC__WEBSOCKET__HPP */
