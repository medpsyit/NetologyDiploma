#include "http_utils.h"

#include <iostream>

#include <boost/beast/core.hpp>
#include <boost/beast/ssl.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/version.hpp>
#include <boost/asio/connect.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/ssl.hpp>
#include <boost/regex.hpp>
#include <boost/locale.hpp>
#include <openssl/ssl.h>

namespace beast = boost::beast;
namespace http = beast::http;
namespace net = boost::asio;
namespace ip = boost::asio::ip;
namespace ssl = boost::asio::ssl;

using tcp = boost::asio::ip::tcp;

bool isText(const boost::beast::multi_buffer::const_buffers_type& b)
{
	for (auto itr = b.begin(); itr != b.end(); itr++)
	{
		for (int i = 0; i < (*itr).size(); i++)
		{
			if (*((const char*)(*itr).data() + i) == 0)
			{
				return false;
			}
		}
	}

	return true;
}

Link linkExtractFromText(std::string& linkText) {
	Link link;
	if (linkText.find("https://") == 0) {
		link.protocol = ProtocolType::HTTPS;
		linkText.erase(0, 8); // Удаляем "https://"
	}
	else if (linkText.find("http://") == 0) {
		link.protocol = ProtocolType::HTTP;
		linkText.erase(0, 7); // Удаляем "http://"
	}

	auto query_start = linkText.find('/');
	if (query_start != std::string::npos) {
		link.hostName = linkText.substr(0, query_start);
		link.query = linkText.substr(query_start);
	}
	else {
		link.hostName = linkText;
		link.query = "";
	}

	return link;
}

std::string getHtmlContent(const Link& link, thread_pool& pool,
	const std::function<void(const Link&)>& onRedirect)
{
	std::string result;
	try
	{
		std::string host = link.hostName;
		std::string query = link.query;

		net::io_context ioc;

		if (link.protocol == ProtocolType::HTTPS)
		{

			ssl::context ctx(ssl::context::tlsv13_client);
			ctx.set_default_verify_paths();

			beast::ssl_stream<beast::tcp_stream> stream(ioc, ctx);
			stream.set_verify_mode(ssl::verify_none);

			stream.set_verify_callback([](bool preverified, ssl::verify_context& ctx) {
				return true; // Accept any certificate
				});


			if (!SSL_set_tlsext_host_name(stream.native_handle(), host.c_str())) {
				beast::error_code ec{static_cast<int>(::ERR_get_error()), net::error::get_ssl_category()};
				throw beast::system_error{ec};
			}

			ip::tcp::resolver resolver(ioc);
			auto endpoints = resolver.resolve(host, "https");
			get_lowest_layer(stream).connect(endpoints); // заработало только с endpoints
			get_lowest_layer(stream).expires_after(std::chrono::seconds(30));


			http::request<http::empty_body> req{http::verb::get, query, 11};
			req.set(http::field::host, host);
			req.set(http::field::user_agent, BOOST_BEAST_VERSION_STRING);

			stream.handshake(ssl::stream_base::client);
			http::write(stream, req);

			beast::flat_buffer buffer;
			http::response<http::dynamic_body> res;
			try {
				http::read(stream, buffer, res);
			}
			catch (const beast::system_error& e) {
				std::cerr << "Error during http::read: " << e.what() << std::endl;
				return result;
			}

			int status_code = res.result_int();

			if (status_code == 200) {
				if (isText(res.body().data())) {
					result = buffers_to_string(res.body().data());
					result = adaptationText(res, result);
				}
				else {
					std::cout << "This is not a text link, bailing out..." << std::endl;
				}
			}
			else {
				if (status_code == 301 || status_code == 302 || status_code == 307 || status_code == 308) {
					auto it = res.find("Location");
					if (it != res.end()) {
						std::string finalUrl = std::string(it->value());
						Link newLink = linkExtractFromText(finalUrl);
						onRedirect(newLink); // redirect
					}
				}
				else if (status_code >= 400 && status_code < 500) {
					throw std::runtime_error("Client error: " + std::to_string(status_code));
				}
				else if (status_code >= 500) {
					throw std::runtime_error("Server error: " + std::to_string(status_code));
				}
			}

			beast::error_code ec;
			stream.shutdown(ec);
			if (ec == net::error::eof) {
				ec = {};
			}

			if (ec) {
				throw beast::system_error{ec};
			}
		}
		else
		{
			tcp::resolver resolver(ioc);
			beast::tcp_stream stream(ioc);

			auto const results = resolver.resolve(host, "http");

			stream.connect(results);

			http::request<http::string_body> req{http::verb::get, query, 11};
			req.set(http::field::host, host);
			req.set(http::field::user_agent, BOOST_BEAST_VERSION_STRING);

			http::write(stream, req);

			beast::flat_buffer buffer;
			http::response<http::dynamic_body> res;
			try {
				http::read(stream, buffer, res);
			}
			catch (const beast::system_error& e) {
				std::cerr << "Error during http::read: " << e.what() << std::endl;
				return result;
			}

			int status_code = res.result_int();

			if (status_code == 200) {
				if (isText(res.body().data())) {
					result = buffers_to_string(res.body().data());
					result = adaptationText(res, result);
				}
				else {
					std::cout << "This is not a text link, bailing out..." << std::endl;
				}
			}
			else {
				if (status_code == 301 || status_code == 302 || status_code == 307 || status_code == 308) {

					auto it = res.find("Location");
					if (it != res.end()) {
						std::string finalUrl = std::string(it->value());
						Link newLink = linkExtractFromText(finalUrl);
						std::cout << "redirect to: " << finalUrl << std::endl;
						onRedirect(newLink);
					}
				}
				else if (status_code >= 400 && status_code < 500) {
					throw std::runtime_error("Client error: " + std::to_string(status_code));
				}
				else if (status_code >= 500) {
					throw std::runtime_error("Server error: " + std::to_string(status_code));
				}
			}

			beast::error_code ec;
			stream.socket().shutdown(tcp::socket::shutdown_both, ec);

			if (ec && ec != beast::errc::not_connected)
				throw beast::system_error{ec};

		}
	}
	catch (const beast::system_error& e) {
		std::cerr << "System error: " << e.code().message() << std::endl;
	}
	catch (const std::exception& e)
	{
		std::cout << e.what() << std::endl;
	}

	return result;
}

std::vector<Link> extractLinks(const std::string& html, const Link& currLink) {

	std::vector<Link> links;

	boost::regex regex(R"(<a\s+(?:[^>]*?\s+)?href=["']([^"']+)["'][^>]*>(.*?)<\/a>)");
	boost::smatch match;

	auto iter = html.begin();
	auto end = html.end();

	bool relative = false;

	try {
		while (boost::regex_search(iter, end, match, regex)) {
			std::string url = match[1]; // Значение href

			// Игнорируем сноски
			if (url.find('#') != std::string::npos) {
				iter = match[0].second;
				continue;
			}
			// Извлечение протокола и хоста
			Link link;
			// Проверяем, есть ли протокол в URL
			if (url.find("https://") == 0) {
				link.protocol = ProtocolType::HTTPS;
				url.erase(0, 8); // Удаляем "https://"
			}
			else if (url.find("http://") == 0) {
				link.protocol = ProtocolType::HTTP;
				url.erase(0, 7); // Удаляем "http://"
			}
			else if (url.find("//") == 0) {
				link.protocol = currLink.protocol; // устанавливаем текущий протокол
				url.erase(0, 2); // Удаляем "//"
			}
			else {
				// Если ссылка относительная
				link.protocol = currLink.protocol;
				link.hostName = currLink.hostName;

				relative = true;
			}

			auto query_start = url.find('/');

			if (relative) {
				link.query = url;
				relative = false;
			}
			else {
				if (query_start != std::string::npos && url.back() != '/') {
					link.hostName = url.substr(0, query_start);
					link.query = url.substr(query_start);
				}
				else {
					link.hostName = (url.back() == '/') ? url.substr(0, url.size() - 1) : url;
					link.query = "/";
				}
			}

			if (!link.hostName.empty() && !link.query.empty()) { // Если ссылка корректная
				links.push_back(link);
			}
			else {
				std::cout << "link skipped: " << getLinkText(link) << std::endl;
			}
			iter = match[0].second;
		}

	}
	catch (const std::exception& e) {
		std::cout << e.what() << std::endl;
	}

	return links;

}

std::string convertEncoding(const std::string& input, const std::string& fromEncoding, const std::string& toEncoding) {
	return boost::locale::conv::between(input, toEncoding, fromEncoding);
}

std::string adaptationText(const boost::beast::http::response<http::dynamic_body>& res, const std::string& result)
{

	std::string contentType = res[http::field::content_type];
	std::string encoding = "UTF-8"; // Значение по умолчанию

	// Поиск кодировки в заголовке
	if (contentType.find("charset=") != std::string::npos)
	{
		size_t start = contentType.find("charset=") + 8;
		size_t end = contentType.find_first_of(" \r\n", start);
		encoding = contentType.substr(start, end - start); // Извлекаем только кодировку
	}

	// Альтернативная проверка в мета-тегах HTML
	if (result.find("<meta charset=\"") != std::string::npos) {
		size_t start = result.find("<meta charset=\"") + 15;
		size_t end = result.find("\"", start);
		encoding = result.substr(start, end - start);
	}

	std::string convertedContent = convertEncoding(result, encoding, "WINDOWS-1251");

	return convertedContent;
}

std::string getLinkText(const Link& link)
{
	std::string textLink;
	if (link.protocol == ProtocolType::HTTPS) {
		textLink = "https://";
	}
	else if (link.protocol == ProtocolType::HTTP) {
		textLink = "http://";
	}
	else {
		return "incorrent link";
	}

	textLink = textLink + link.hostName + link.query;

	return textLink;
}
