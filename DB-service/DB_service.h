#pragma once

#include <iostream>
#include <vector>
#include <mutex>
#include <pqxx/pqxx>
#include "../Config/config.h"

class DB_Handle {
public:
	DB_Handle(const Config::DataBase& db);
	~DB_Handle();

	int add_link(const std::string& url);
	int add_word(const std::string& word);
	void add_frequency(int link_id, int word_id, int frequency);
	std::vector<std::string> get_query_result(const std::vector<std::string>& words);

	void commit();

private:
	std::string connection_string;
	pqxx::connection* connection = nullptr;
	std::mutex dbMutex;

	void initialize();
};