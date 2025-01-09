
#define EXC_USE_WEBSOCKET_IMPL
#include <wbx/exc/WebsocketImpl.hpp>
#undef EXC_USE_WEBSOCKET_IMPL

#include <cstdio>
#include <wbx/exc/RootCerts.hpp>

namespace wbx {
namespace exc {

WebsocketImplSession::WebsocketImplSession(net::io_context &ioc,
					   ssl::context &ctx):
	ws_(net::make_strand(ioc), ctx),
	resolver_(net::make_strand(ioc)),
	nr_read_after_(0)
{
}

void WebsocketImplSession::run(void)
{
	resolver_.async_resolve(host_, std::to_string(port_),
				beast::bind_front_handler(
					&WebsocketImplSession::onResolve,
					shared_from_this()));
}

void WebsocketImplSession::onResolve(beast::error_code ec,
				     tcp::resolver::results_type results)
{
	if (ec)
		return;

	beast::get_lowest_layer(ws_).expires_after(std::chrono::seconds(60));

	// Make the connection on the IP address we get from a lookup
	beast::get_lowest_layer(ws_)
		.async_connect(results, beast::bind_front_handler(
				&WebsocketImplSession::onConnect,
				shared_from_this()));
}

void WebsocketImplSession::onConnect(beast::error_code ec,
				     tcp::resolver::results_type::endpoint_type ep)
{
	const char *host;

	if (ec)
		return;

	beast::get_lowest_layer(ws_).expires_after(std::chrono::seconds(60));

	host = host_.c_str();
	if (!SSL_set_tlsext_host_name(ws_.next_layer().native_handle(), host)) {
		// ec = beast::error_code(static_cast<int>(::ERR_get_error()),
		// 			net::error::get_ssl_category());
		return;
	}

	host_ += ':' + std::to_string(ep.port());
	ws_.next_layer()
		.async_handshake(ssl::stream_base::client,
				 beast::bind_front_handler(
					&WebsocketImplSession::onSslHandshake,
					shared_from_this()));
}

void WebsocketImplSession::onSslHandshake(beast::error_code ec)
{
	if (ec)
		return;

	beast::get_lowest_layer(ws_).expires_never();

	ws_.set_option(
		websocket::stream_base::timeout::suggested(
			beast::role_type::client));

	ws_.set_option(
		websocket::stream_base::decorator(
			[ua=user_agent_](websocket::request_type& req) {
				req.set(http::field::user_agent, ua);
			}));

	ws_.async_handshake(host_, uri_,
			beast::bind_front_handler(
				&WebsocketImplSession::onHandshake,
				shared_from_this()));
}

inline
void PendingWrites::__push(const std::string &data)
{
	std::lock_guard<std::mutex> lock(mtx_);

	data_.push(data);
}

inline
std::string PendingWrites::__pop(void)
{
	std::lock_guard<std::mutex> lock(mtx_);
	std::string data = data_.front();
	data_.pop();
	return data;
}

void PendingWrites::push(const std::string &data)
{
	std::lock_guard<std::mutex> lock(mtx_);
	__push(data);
}

inline
std::string PendingWrites::pop(void)
{
	std::lock_guard<std::mutex> lock(mtx_);
	return __pop();
}

void WebsocketImplSession::popWrite(void)
{
	std::lock_guard<std::mutex> lock(pw_.mtx_);

	if (pw_.__size()) {
		ws_.async_write(net::buffer(pw_.__pop()),
				beast::bind_front_handler(
					&WebsocketImplSession::onWrite,
					shared_from_this()));
	}
}

void WebsocketImplSession::onHandshake(beast::error_code ec)
{
	if (ec)
		return;

	is_connected_ = true;
	if (onConnect_)
		onConnect_(this, udata_);

	popWrite();
}

void WebsocketImplSession::onWrite(beast::error_code ec,
				   std::size_t bytes_transferred)
{
	if (ec)
		return;

	if (onWrite_)
		onWrite_(this, bytes_transferred, udata_);

	popWrite();
}

void WebsocketImplSession::onRead(beast::error_code ec,
				  std::size_t bytes_transferred)
{
	if (ec)
		return;

	if (onRead_) {
		const char *buf = reinterpret_cast<const char *>(buffer_.data().data());
		size_t len = buffer_.size();
		buffer_.consume(onRead_(this, buf, len, udata_));
	} else {
		buffer_.consume(bytes_transferred);
	}

	if (nr_read_after_.fetch_sub(1) > 0)
		read();
	else
		nr_read_after_.fetch_add(1);

	popWrite();
}

void WebsocketImplSession::onClose(beast::error_code ec)
{
	if (onClose_)
		onClose_(this, udata_);
}

WebsocketImplSession::~WebsocketImplSession(void) = default;

WebsocketImpl::WebsocketImpl(void):
	io_ctx_(),
	ssl_ctx_(ssl::context::tlsv12_client)
{
	load_root_certificates(ssl_ctx_);
}

WebsocketImpl::~WebsocketImpl(void) = default;

std::shared_ptr<WebsocketImplSession>
WebsocketImpl::createSession(void)
{
	return std::make_shared<WebsocketImplSession>(io_ctx_, ssl_ctx_);
}

} /* namespace exc */
} /* namespace wbx */
