#include "parser.h"

#include <iostream>
#include <boost/locale.hpp>
#include <boost/regex.hpp>

void getWords(std::unordered_map<std::string, int>& wordsCount, const std::string& html)
{
	try {
		// Удаляем HTML-теги
		std::string text = boost::regex_replace(html, boost::regex(R"(<[^>]+>)"), " ");

		// Приводим все к нижнему регистру
		text = boost::locale::to_lower(text, boost::locale::generator().generate(""));

		// Регулярное выражение для нахождения слов (последовательностей символов)
		boost::regex expression(R"(\b[a-zA-Z]+\b)");

		boost::sregex_iterator it(text.begin(), text.end(), expression);
		boost::sregex_iterator end;

		while (it != end) {
			std::string word = it->str();
			wordsCount[word]++;
			++it;
		}
	}
	catch (const std::exception& e) {
		std::cout << e.what() << std::endl;
	}
}
