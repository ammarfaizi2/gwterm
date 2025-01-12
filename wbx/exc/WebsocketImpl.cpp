
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
	beast::get_lowest_layer(ws_).async_connect(results,
			beast::bind_front_handler(&WebsocketImplSession::onConnect,
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
	if (!SSL_set_tlsext_host_name(ws_.next_layer().native_handle(), host))
		return;

	host_ += ':' + std::to_string(ep.port());
	ws_.next_layer().async_handshake(ssl::stream_base::client,
		beast::bind_front_handler(
				&WebsocketImplSession::onSslHandshake,
				shared_from_this()));
}

void WebsocketImplSession::onSslHandshake(beast::error_code ec)
{
	if (ec)
		return;

	beast::get_lowest_layer(ws_).expires_never();

	ws_.set_option(websocket::stream_base::timeout::suggested(
			beast::role_type::client));

	ws_.set_option(websocket::stream_base::decorator(
		[ua = user_agent_](websocket::request_type& req)
		{
			req.set(http::field::user_agent, ua);
		}));

	ws_.async_handshake(host_, uri_, beast::bind_front_handler(
		&WebsocketImplSession::onHandshake, shared_from_this()));
}

void WebsocketImplSession::onHandshake(beast::error_code ec)
{
	if (ec)
		return;

	if (onConnect_)
		onConnect_(this, udata_);

	std::lock_guard<std::mutex> lock(wq_mtx_);
	if (!write_queue_.empty()) {
		struct write_buf &wb = write_queue_.front();
		ws_.async_write(net::buffer(wb.data(), wb.len()),
				beast::bind_front_handler(
					&WebsocketImplSession::onWrite,
					shared_from_this()));
	}
}

void WebsocketImplSession::onWrite(beast::error_code ec,
				   std::size_t bytes_transferred)
{
	if (ec)
		return;

	if (onWrite_)
		onWrite_(this, bytes_transferred, udata_);

	std::lock_guard<std::mutex> lock(wq_mtx_);
	write_queue_.pop();
	if (!write_queue_.empty()) {
		struct write_buf &wb = write_queue_.front();
		ws_.async_write(net::buffer(wb.data(), wb.len()),
				beast::bind_front_handler(
					&WebsocketImplSession::onWrite,
					shared_from_this()));
	} else {
		popNrRead();
	}
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

	std::lock_guard<std::mutex> lock(wq_mtx_);
	if (!write_queue_.empty()) {
		struct write_buf &wb = write_queue_.front();
		ws_.async_write(net::buffer(wb.data(), wb.len()),
				beast::bind_front_handler(
					&WebsocketImplSession::onWrite,
					shared_from_this()));
	} else {
		popNrRead();
	}
}

void WebsocketImplSession::onClose(beast::error_code ec)
{
	(void)ec;
	if (onClose_)
		onClose_(this, udata_);
}

bool WebsocketImplSession::popNrRead(void)
{
	if (nr_read_after_.fetch_sub(1) > 0) {
		read();
		return true;
	} else {
		nr_read_after_.fetch_add(1);
		return false;
	}
}

void WebsocketImplSession::write(const void *data, size_t len)
{
	struct write_buf wb;

	if (!wb.set(data, len))
		throw std::bad_alloc();

	std::lock_guard<std::mutex> lock(wq_mtx_);
	write_queue_.push(std::move(wb));
}

void WebsocketImplSession::read(void)
{
	ws_.async_read(buffer_,
		beast::bind_front_handler(
			&WebsocketImplSession::onRead,
			shared_from_this()));
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
