#pragma once 

#include <string>
#include <mutex>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/ini_parser.hpp>

class Config {

public:
    // База Данных
    struct DataBase {
        std::string host;
        std::string port;
        std::string name;
        std::string login;
        std::string pass;
    };

    // Клиент ("Паук")
    struct Spider {
        std::string mainLink;
        std::string depth;
    };

    // Сервер (Поисковик)
    struct Server {
        std::string port;
    };

private:
    // Приватный конструктор
    Config() {}

    std::mutex mutex_;

    DataBase database_;
    Spider spider_;
    Server server_;

public:

    // Получение экземпляра
    static Config& getInstance();

    // Метод для инициализации конфигурации
    void initialize(const std::string& filename);

    // Доступ к параметрам конфигурации
    const DataBase& getDataBaseSettings() const { return database_; }
    const Spider& getSpiderSettings() const { return spider_; }
    const Server& getServerSettings() const { return server_; }

    // Удаляем методы копирования и перемещения
    Config(const Config&) = delete;
    Config& operator=(const Config&) = delete;

};