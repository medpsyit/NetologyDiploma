#pragma once 
#include <vector>
#include <string>
#include <boost/beast/http.hpp>
#include "link.h"

class thread_pool;

std::string getHtmlContent(const Link& link, thread_pool& pool,
	const std::function<void(const Link&)>& onRedirect);

std::vector<Link> extractLinks(const std::string& html, const Link& currLink);

std::string convertEncoding(const std::string& input, const std::string& fromEncoding, const std::string& toEncoding);

std::string adaptationText(const boost::beast::http::response<boost::beast::http::dynamic_body>& res, const std::string& result);

std::string getLinkText(const Link& link);

Link linkExtractFromText(std::string& linkText);
