#pragma once

#include <unordered_map>
#include <boost/beast/http.hpp>

void getWords(std::unordered_map<std::string, int>& wordsCount, const std::string& html);