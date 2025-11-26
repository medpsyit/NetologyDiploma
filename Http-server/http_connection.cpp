#include "http_connection.h"

#include <sstream>
#include <iomanip>
#include <locale>
#include <codecvt>
#include <iostream>

#include <boost/locale.hpp>
#include <boost/regex.hpp>

namespace beast = boost::beast;
namespace http = beast::http;
namespace net = boost::asio;
using tcp = boost::asio::ip::tcp;

std::vector<std::string> splitString(const std::string& str, char delimiter) {

	if (str.empty()) {
		throw std::runtime_error("Empty search attempt!");
	}

	std::vector<std::string> result;
	std::stringstream ss(str);
	std::string item;

	while (std::getline(ss, item, delimiter)) {
		result.push_back(item);
	}

	return result;
}

std::string url_decode(const std::string& encoded) {
	std::string res;
	std::istringstream iss(encoded);
	char ch;

	while (iss.get(ch)) {
		if (ch == '%') {
			int hex;
			iss >> std::hex >> hex;
			res += static_cast<char>(hex);
		}
		else {
			res += ch;
		}
	}

	return res;
}

std::string convert_to_utf8_lower(const std::string& str) {
	std::string url_decoded = boost::locale::to_lower(url_decode(str), boost::locale::generator().generate(""));
	return url_decoded;
}

HttpConnection::HttpConnection(tcp::socket socket, const Config::DataBase& db)
	: socket_(std::move(socket)), database_({db})
{
}

//HttpConnection::HttpConnection(tcp::socket socket, std::shared_ptr<DB_Handle> db)
//	: socket_(std::move(socket)), database_(std::move(db))
//{
//}

//HttpConnection::HttpConnection(tcp::socket socket)
//	: socket_(std::move(socket))
//{
//}


void HttpConnection::start()
{
	readRequest();
	checkDeadline();
}


void HttpConnection::readRequest()
{
	auto self = shared_from_this();

	http::async_read(
		socket_,
		buffer_,
		request_,
		[self](beast::error_code ec,
			std::size_t bytes_transferred)
		{
			boost::ignore_unused(bytes_transferred);
			if (!ec)
				self->processRequest();
		});
}

void HttpConnection::processRequest()
{
	response_.version(request_.version());
	response_.keep_alive(false);

	switch (request_.method())
	{
	case http::verb::get:
		response_.result(http::status::ok);
		response_.set(http::field::server, "Beast");
		createResponseGet();
		break;
	case http::verb::post:
		response_.result(http::status::ok);
		response_.set(http::field::server, "Beast");
		createResponsePost();
		break;

	default:
		response_.result(http::status::bad_request);
		response_.set(http::field::content_type, "text/plain");
		beast::ostream(response_.body())
			<< "Invalid request-method '"
			<< std::string(request_.method_string())
			<< "'";
		break;
	}

	writeResponse();
}


void HttpConnection::createResponseGet()
{
	try {
		if (request_.target() == "/")
		{
			response_.set(http::field::content_type, "text/html");
			beast::ostream(response_.body())
				<< "<html>\n"
				<< "<head><meta charset=\"UTF-8\"><title>Search Engine</title></head>\n"
				<< "<body>\n"
				<< "<h1>Search Engine</h1>\n"
				<< "<p>Welcome!<p>\n"
				<< "<form action=\"/\" method=\"post\">\n"
				<< "    <label for=\"search\">Search:</label><br>\n"
				<< "    <input type=\"text\" id=\"search\" name=\"search\"><br>\n"
				<< "    <input type=\"submit\" value=\"Search\">\n"
				<< "</form>\n"
				<< "</body>\n"
				<< "</html>\n";
		}
		else
		{
			throw std::runtime_error("File not found");
		}
	}
	catch (const std::exception& e) {
		response_.result(http::status::internal_server_error);
		response_.set(http::field::content_type, "text/html");
		beast::ostream(response_.body())
			<< "<html>\n"
			<< "<head><meta charset=\"UTF-8\"><title>Error</title></head>\n"
			<< "<body>\n"
			<< "<h1>Error</h1>\n"
			<< "<p>" << e.what() << "</p>\n"
			<< "<a href=\"/\">Back to Search</a>\n" 
			<< "</body>\n"
			<< "</html>\n";
	}

}

void HttpConnection::createResponsePost()
{
	try {

		if (request_.target() == "/")
		{
			std::string s = buffers_to_string(request_.body().data());

			size_t pos = s.find('=');
			if (pos == std::string::npos)
			{
				throw std::runtime_error("Invalid request format");
			}

			std::string key = s.substr(0, pos);
			std::string value = s.substr(pos + 1);
			std::string utf8value = convert_to_utf8_lower(value);

			if (key != "search")
			{
				throw std::runtime_error("Invalid search key");
			}

			utf8value = boost::regex_replace(utf8value, boost::regex(R"([^a-zA-Z\+])"), "");
			std::vector<std::string> words = splitString(utf8value, '+');
			std::vector<std::string> searchResult = database_.get_query_result(words);

			response_.set(http::field::content_type, "text/html");
			beast::ostream(response_.body())
				<< "<html>\n"
				<< "<head><meta charset=\"UTF-8\"><title>Search Engine</title></head>\n"
				<< "<body>\n"
				<< "<h1>Search Engine</h1>\n"
				<< "<p>Response:<p>\n"
				<< "<ul>\n";

			if (searchResult.empty()) {
				beast::ostream(response_.body())
					<< "<p>Could not find pages with this content!<p>\n";
			}
			else {
				for (const auto& url : searchResult) {
					beast::ostream(response_.body())
						<< "<li><a href=\"" << url << "\">" << url << "</a></li>";
				}
			}

			beast::ostream(response_.body())
				<< "</ul>\n"
				<< "<form onsubmit=\"return false;\">\n"
				<< "<button type=\"button\" onclick=\"window.location.href='/'\">Back to Search</button>\n"
				<< "</form>\n"
				<< "</body>\n"
				<< "</html>\n";
		}
		else
		{
			throw std::runtime_error("File not found");
		}

	}
	catch (const std::exception& e) {
	response_.result(http::status::internal_server_error);
	response_.set(http::field::content_type, "text/html");
	beast::ostream(response_.body())
		<< "<html>\n"
		<< "<head><meta charset=\"UTF-8\"><title>Error</title></head>\n"
		<< "<body>\n"
		<< "<h1>Error</h1>\n"
		<< "<p>" << e.what() << "</p>\n" 
		<< "<a href=\"/\">Back to Search</a>\n" 
		<< "</body>\n"
		<< "</html>\n";
	}
}


void HttpConnection::writeResponse()
{
	auto self = shared_from_this();

	response_.content_length(response_.body().size());

	http::async_write(
		socket_,
		response_,
		[self](beast::error_code ec, std::size_t)
		{
			self->socket_.shutdown(tcp::socket::shutdown_send, ec);
			self->deadline_.cancel();
		});
}

void HttpConnection::checkDeadline()
{
	auto self = shared_from_this();

	deadline_.async_wait(
		[self](beast::error_code ec)
		{
			if (!ec)
			{
				self->socket_.close(ec);
			}
		});
}

