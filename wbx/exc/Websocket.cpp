// SPDX-License-Identifier: GPL-2.0-only

#define EXC_USE_WEBSOCKET_IMPL
#include <wbx/exc/Websocket.hpp>
#include <wbx/exc/RootCerts.hpp>

namespace wbx {
namespace exc {

WebsocketSession::WebsocketSession(Websocket *ws, const std::string &host,
				   uint16_t port, const std::string &uri):
	ws_(ws)
{
	ws_sess_shr_ = new std::shared_ptr<WebsocketImplSession>();
	*ws_sess_shr_ = std::make_shared<WebsocketImplSession>(ws->getIOCtx(),
							       ws->getSSLCtx());

	ws_sess_ = ws_sess_shr_->get();
	ws_sess_->setHost(host);
	ws_sess_->setPort(port);
	ws_sess_->setUri(uri);
	ws_sess_->setUData(this);
}

void WebsocketSession::setOnConnect(WsOnConnect_t onConnect)
{
	ws_sess_->setOnConnect([ocf=std::move(onConnect)](
				WebsocketImplSession *ws_sess,
				void *udata) {
		WebsocketSession *ws = static_cast<WebsocketSession *>(udata);
		ocf(ws);
		(void)ws_sess;
	});
}

void WebsocketSession::setOnRead(WsOnRead_t onRead)
{
	ws_sess_->setOnRead([orf=std::move(onRead)](
				WebsocketImplSession *ws_sess,
				const char *data,
				size_t len, void *udata) {
		WebsocketSession *ws = static_cast<WebsocketSession *>(udata);
		(void)ws_sess;
		return orf(ws, data, len);
	});
}

void WebsocketSession::setOnWrite(WsOnWrite_t onWrite)
{
	ws_sess_->setOnWrite([owf=std::move(onWrite)](
				WebsocketImplSession *ws_sess, size_t len,
				void *udata) {
		WebsocketSession *ws = static_cast<WebsocketSession *>(udata);
		owf(ws, len);
		(void)ws_sess;
	});
}

void WebsocketSession::setOnClose(WsOnClose_t onClose)
{
	ws_sess_->setOnClose([ocf=std::move(onClose)](
					WebsocketImplSession *ws_sess,
					void *udata) {
		WebsocketSession *ws = static_cast<WebsocketSession *>(udata);
		ocf(ws);
		(void)ws_sess;
	});
}

void WebsocketSession::setOnConnErr(WsOnConnErr_t onConnErr)
{
	ws_sess_->setOnConnErr([ocef=std::move(onConnErr)](
					WebsocketImplSession *ws_sess, int code,
					const char *msg, void *udata) {
		WebsocketSession *ws = static_cast<WebsocketSession *>(udata);
		ocef(ws, code, msg);
	});
}

void WebsocketSession::setHost(const std::string &host)
{
	ws_sess_->setHost(host);
}

void WebsocketSession::setPort(uint16_t port)
{
	ws_sess_->setPort(port);
}

void WebsocketSession::setUri(const std::string &uri)
{
	ws_sess_->setUri(uri);
}

void WebsocketSession::setUserAgent(const std::string &userAgent)
{
	ws_sess_->setUserAgent(userAgent);
}

void WebsocketSession::write(const char *data, size_t len)
{
	ws_sess_->write(data, len);
}

void WebsocketSession::read(void)
{
	ws_sess_->readAfter();
}

void WebsocketSession::run(void)
{
	ws_sess_->run();
}

void WebsocketSession::close(void)
{
	ws_sess_->close();
}

WebsocketSession::~WebsocketSession(void)
{
	delete ws_sess_shr_;
}

Websocket::Websocket(void):
	ws_(new WebsocketImpl())
{
}

Websocket::~Websocket(void)
{
	if (ws_thread_)
		ws_thread_->join();

	delete ws_;
}

WebsocketSession *Websocket::createSession(const std::string &host,
					   uint16_t port,
					   const std::string &uri)
{
	std::unique_ptr<WebsocketSession> ws_sess;

	ws_sess = std::make_unique<WebsocketSession>(this, host, port, uri);
	WebsocketSession *ws_sess_ptr = ws_sess.get();
	ws_sessions_.push_back(std::move(ws_sess));
	return ws_sess_ptr;
}

void Websocket::run(void)
{
	ws_->run();
}

void Websocket::bgRun(void)
{
	ws_thread_ = std::make_unique<std::thread>([this]() {
		ws_->run();
	});
}

} /* namespace exc */
} /* namespace wbx */
