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
private:
	tcp::socket socket_;

};
int main(){
	
}