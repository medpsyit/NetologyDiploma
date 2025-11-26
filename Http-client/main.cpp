#include <iostream>
#include <vector>
#include <thread>
#include <mutex>
#include <queue>
#include <condition_variable>
#include <functional>

#include <boost/asio.hpp>

#include "http_utils.h"
#include "parser.h"
#include "../DB-service/DB_service.h"

template <class T>
class safe_queue {
private:
	std::queue<T> task_list;
	std::mutex m;
	std::condition_variable condition;
	std::atomic<bool> done{ false };
public:
	void push(T&& f_obj) {
		{
			std::unique_lock<std::mutex> lock(m);
			task_list.push(f_obj);
		}
		condition.notify_one();
	}
	void pop() {
		std::function<void()> task;
		{
			std::unique_lock<std::mutex> lock(m);
			auto timeout = std::chrono::steady_clock::now() + std::chrono::seconds(2);
			if (condition.wait_until(lock, timeout, [this] { return !task_list.empty(); })) {
				if (!task_list.empty()) {
					task = std::move(task_list.front());
					task_list.pop();
				}
			}
			else {
				if (task_list.empty()) { done = true; }
			}
		}
		if (task) {
			task(); // Выполняем задачу
		}
	}
	void finish() {
		done = true; // Устанавливаем флаг завершения
		condition.notify_all(); // Уведомляем все потоки
	}
	bool is_done() const {
		return done;
	}
	bool empty() const {
		return task_list.empty();
	}
	~safe_queue() {
		std::cout << "Destructing safe_queue" << std::endl;
	}
};

class thread_pool {
private:
	std::vector<std::thread> VT{};
	safe_queue<std::function<void()>> SQ{};
	const size_t cores{ std::thread::hardware_concurrency() - 2 };
	std::atomic<bool> done{ false };
public:
	thread_pool() {
		for (size_t i = 0; i < cores; i++) {
			VT.push_back(std::thread(&thread_pool::work, this));
		}
	}
	void work() {
		 while (!SQ.is_done()){
				SQ.pop();
		}
	}
	void submit(std::function<void()>&& f_obj) {
		SQ.push(std::move(f_obj));
	}

	bool task_empty() {
		return SQ.empty();
	}

	void finish() {
		for (auto& thread : VT) {
			if (thread.joinable()) {
				thread.join(); // Дожидаемся окончания работы всех потоков
			}
		}
	}

	~thread_pool() {
		finish();
		std::cout << "Destructing thread_pool" << std::endl;
	}
};

void parseLink(thread_pool& pool, const Link& link, int depth, std::shared_ptr<DB_Handle> db)
{
	try {

		std::string html = getHtmlContent(link);

		if (html.size() == 0)
		{
			std::cout << "Failed to get HTML Content for: " << link.hostName << link.query << std::endl;
			return;
		}

		std::string linkText = getLinkText(link);
		int link_id = db->add_link(linkText);
		int word_id;

		std::unordered_map<std::string, int> wordsCount;
		getWords(wordsCount, html);
		for (const auto& [word, count] : wordsCount) {
			word_id = db->add_word(word);
			db->add_frequency(link_id, word_id, count);
		}

		if (depth > 0) {

			std::vector<Link> links = extractLinks(html);

			for (auto& subLink : links) {
				pool.submit([&pool, subLink, depth, db]() { parseLink(pool, subLink, depth - 1, db); });
			}
		}

	}
	catch (const std::exception& e)
	{
		std::cout << e.what() << std::endl;
	}

}

int main()
{

	SetConsoleCP(CP_UTF8);
	SetConsoleOutputCP(CP_UTF8);

	try {
		thread_pool test;

		Config::getInstance().initialize("../config.ini");
		const auto& dbSettings = Config::getInstance().getDataBaseSettings();
		auto currDB = std::make_shared<DB_Handle>(dbSettings); // чтобы не уничтожалась, пока потоки с ней работают

		const auto& spiderSettings = Config::getInstance().getSpiderSettings();
		Link link = Link::parse(spiderSettings.mainLink);
		int depth = std::stoi(spiderSettings.depth);

		test.submit([&test, link, depth, currDB]() { parseLink(test, link, depth, currDB); });
	}
	catch (const std::exception& e)
	{
		std::cout << e.what() << std::endl;
	}
	return 0;
}
