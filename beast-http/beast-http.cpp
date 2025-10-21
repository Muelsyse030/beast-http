#include <boost/beast/http.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/version.hpp>
#include <boost/asio.hpp>
#include <chrono>
#include <ctime>
#include <cstdlib>
#include <iostream>
#include <memory>
#include <string>
#include <Json/json.h>
#include <Json/value.h>
#include <Json/reader.h>

namespace beast = boost::beast;
namespace http = beast::http;
namespace net = boost::asio;
using tcp = boost::asio::ip::tcp;

namespace my_program_state {
	std::size_t request_count() {
		static std::size_t count = 0;
		return ++count;
	}

	std::time_t now() {
		return std::time(0);
	}
}
class http_connection : public std::enable_shared_from_this<http_connection> {
public:
	http_connection(tcp::socket socket) : socket_(std::move(socket)){
		
	}
	void start() {
		read_request();
		check_deadline();
	}
private:
	tcp::socket socket_;
	beast::flat_buffer buffer_ {8192};
	http::request<http::dynamic_body> request_;
	http::response<http::dynamic_body> response_;

	net::steady_timer deadline_{socket_.get_executor(),std::chrono::seconds(60)};

	void read_request() {
		auto self = shared_from_this();
		http::async_read(socket_, buffer_, request_, [self](beast::error_code ec , std::size_t bytes_transferred) {
			boost::ignore_unused(bytes_transferred);
			if (!ec) {
				self->process_request();
			}
			});

	}

	void check_deadline() {
		auto self = shared_from_this();
		deadline_.async_wait([self](boost::system::error_code ec) {
			if (!ec) {
				self->socket_.close();
			}
		});
	}
	void process_request() {
		response_.version(request_.version());
		response_.keep_alive(false);
		switch (request_.method()) {
			case http::verb::get:
				response_.result(http::status::ok);
				response_.set(http::field::server, "beast");
				create_response();
				response_.result(http::status::ok);
				response_.set(http::field::server, "beast");
				create_post_response();
				break;
			case http::verb::post:
				break;
			default:
				response_.result(http::status::bad_request);
				response_.set(http::field::content_type, "text/plain");
				beast::ostream(response_.body()) << "invaild request-method" << 
					std::string(request_.method_string()) << " ";
				break;
		}
		write_response();
	}
	void create_response() {
		if (request_.target() == "/count") {
			response_.set(http::field::content_type, "text/html");
			beast::ostream(request_.body()) << "<html>\n"
				<< "<head><title>Request count</title></head>\n"
				<< "<body>\n"
				<< "<h1>Request count</h1>\n"
				<< "<p>There have been "
				<< my_program_state::request_count()
				<< " requests so far.</p>\n"
				<< "</body>\n"
				<< "</html>\n";
		}
		else if (request_.target() == "/time") {
			response_.set(http::field::content_type, "text/html");
			beast::ostream(response_.body())
				<< "<html>\n"
				<< "<head><title>Current time</title></head>\n"
				<< "<body>\n"
				<< "<h1>Current time</h1>\n"
				<< "<p>The current time is "
				<< my_program_state::now()
				<< " seconds since the epoch.</p>\n"
				<< "</body>\n"
				<< "</html>\n";
		}
		else{
			response_.result(http::status::not_found);
			response_.set(http::field::content_type, "text/html");
			beast::ostream(response_.body()) << "File not found\r\n";
		}
	}
	void write_response() {
		auto self = shared_from_this();
		response_.content_length(response_.body().size());
		http::async_write(socket_, response_, [self](beast::error_code ec, std::size_t) {
			self->socket_.shutdown(tcp::socket::shutdown_send);
			self->deadline_.cancel();
		});
	}
	void create_post_response() {
		if (request_.target() == "/email") {
			auto& body = this->request_.body();
			auto body_str = boost::beast::buffers_to_string(body.data());
			std::cout << "recevie body is" << body_str << std::endl;
			this->response_.set(http::field::content_type, "text/json");
			Json::Value root;
			Json::Reader reader;
			Json::Value src_root;
			reader.parse(body_str, src_root);
			bool parse_success = reader.parse(body_str, src_root);
			if (!parse_success) {
				std::cout << "failed to parse Json data" << std::endl;
				root["error"] = "1001";
				std::string jsonstr = root.toStyledString();
				beast::ostream(response_.body()) << "failed not found";
			}
			auto email = src_root["email"].asString();
			std::cout << "email is" << email << std::endl;
			root["error"] = 0;
			root["email"] = src_root["email"];
			root["msg"] = "recevie email post sucsess";
			std::string jsonstr = root.toStyledString();
			beast::ostream(response_.body()) << jsonstr;
		}
		else {
			response_.result(http::status::not_found);
			response_.set(http::field::content_type, "text/html");
			beast::ostream(response_.body()) << "File not found\r\n";
		}
	}
};
void http_server(tcp::acceptor& acceptor, tcp::socket& socket) {
	acceptor.async_accept(socket, [&](boost::system::error_code ec) {
		if (!ec) {
			std::make_shared<http_connection>(std::move(socket))->start();
		}
		http_server(acceptor, socket);
		});
}
int main(){
	try{
		auto const address = net::ip::make_address("127.0.0.1");
		unsigned short port = static_cast<unsigned short>(8080);
		net::io_context ioc{ 1 };
		tcp::acceptor acceptor{ ioc , {address , port } };
		tcp::socket socket(ioc);
		http_server(acceptor, socket);
		ioc.run();
	}
	catch (std::exception e) {
		std::cerr << "ERROR:" << e.what() << std::endl;
		return EXIT_FAILURE;
	}
}