#include "config.h"
#include <iostream>

// Получение экземпляра
Config& Config::getInstance() {

    static std::mutex mutex; // Мьютекс для защиты доступа
    std::lock_guard<std::mutex> lock(mutex); // Блокировка мьютекса

    static Config instance; // Создает единственный экземпляр
    return instance;

}

void Config::initialize(const std::string& filename) {

    std::lock_guard<std::mutex> lock(mutex_); // Блокировка при инициализации

    boost::property_tree::ptree pt;

    try {
        boost::property_tree::ini_parser::read_ini(filename, pt);

        // Чтение конфигураций
        database_.host = pt.get<std::string>("DataBase.host");
        database_.port = pt.get<std::string>("DataBase.port");
        database_.name = pt.get<std::string>("DataBase.name");
        database_.login = pt.get<std::string>("DataBase.login");
        database_.pass = pt.get<std::string>("DataBase.pass");

        spider_.mainLink = pt.get<std::string>("Spider.main");
        spider_.depth = pt.get<std::string>("Spider.depth");

        server_.port = pt.get<std::string>("Server.port");
    }
    catch (const boost::property_tree::ini_parser_error& e) {
        std::cerr << "Ошибка при открытии INI-файла: " << e.what() << std::endl;
    }
    
}