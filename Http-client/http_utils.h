#pragma once 
#include <vector>
#include <string>
#include <boost/beast/http.hpp>
#include "link.h"

std::string getHtmlContent(const Link& link);

std::vector<Link> extractLinks(const std::string& html);

std::string convertEncoding(const std::string& input, const std::string& fromEncoding, const std::string& toEncoding);

std::string adaptationText(const boost::beast::http::response<boost::beast::http::dynamic_body>& res, const std::string& result);

std::string getLinkText(const Link& link);