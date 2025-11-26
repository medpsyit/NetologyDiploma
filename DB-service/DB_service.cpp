#include "DB_service.h"

DB_Handle::DB_Handle(const Config::DataBase& db) {
    connection_string = "dbname=" + db.name +
        " user=" + db.login +
        " password=" + db.pass +
        " host=" + db.host +
        " port=" + db.port;

    try {
        connection = new pqxx::connection(connection_string);
        if (connection->is_open()) {
            initialize();
            //std::cout << "CONNECTION OK" << std::endl;
        }
        else {
            std::cerr << "CONNECTION ERROR" << std::endl;
        }
    } catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;
    }
}

DB_Handle::~DB_Handle() {
    std::lock_guard<std::mutex> lock(dbMutex);
	delete connection;
}

void DB_Handle::initialize() {

    std::lock_guard<std::mutex> lock(dbMutex);

    pqxx::work work(*connection);

    work.exec("CREATE TABLE IF NOT EXISTS links (id INT GENERATED ALWAYS AS IDENTITY PRIMARY KEY, url VARCHAR UNIQUE NOT NULL);");
    work.exec("CREATE TABLE IF NOT EXISTS words (id INT GENERATED ALWAYS AS IDENTITY PRIMARY KEY, word VARCHAR UNIQUE NOT NULL);");
    work.exec("CREATE TABLE IF NOT EXISTS frequency (link_id INT REFERENCES links(id), "
        "word_id INT REFERENCES words(id), count INT NOT NULL, "
        "UNIQUE (link_id, word_id));");

    connection->prepare("get_specific_word_frequency",
        "SELECT url, SUM(f.count) as sum_words "
        "FROM frequency f "
        "JOIN words w ON f.word_id = w.id "
        "JOIN links l ON f.link_id = l.id "
        "WHERE w.word = ANY($1) "
        "GROUP BY url "
        "ORDER BY sum_words DESC "
        "LIMIT 10;");

    work.commit();

    //std::cout << "Tables created!" << std::endl;
}

int DB_Handle::add_link(const std::string& url) {
    std::lock_guard<std::mutex> lock(dbMutex);
    pqxx::work work(*connection);
    try {
        std::string insert_query = R"(
            WITH ins AS (
                INSERT INTO links (url) VALUES ($1)
                ON CONFLICT (url) DO NOTHING
                RETURNING id
            )
            SELECT id FROM ins
            UNION ALL
            SELECT id FROM links WHERE url = $1
            LIMIT 1;
        )";

        pqxx::result id = work.exec_params(insert_query, url);
        work.commit();

        return id[0][0].as<int>();
    }
    catch (const std::exception& e) {
        work.abort(); 
        std::cerr << "Ошибка при добавлении ссылки: " << e.what() << std::endl;
        return -1; 
    }
}

int DB_Handle::add_word(const std::string& word) {
    std::lock_guard<std::mutex> lock(dbMutex);
    pqxx::work work(*connection); 
    try {
        std::string insert_query = R"(
            WITH ins AS (
                INSERT INTO words (word) VALUES ($1)
                ON CONFLICT (word) DO NOTHING
                RETURNING id
            )
            SELECT id FROM ins
            UNION ALL
            SELECT id FROM words WHERE word = $1
            LIMIT 1;
        )";

        pqxx::result id = work.exec_params(insert_query, word);
        work.commit(); 

        return id[0][0].as<int>();
    }
    catch (const std::exception& e) {
        work.abort(); 
        std::cerr << "Ошибка при добавлении слова: " << e.what() << std::endl;
        return -1; 
    }
}

void DB_Handle::add_frequency(int link_id, int word_id, int frequency) {
    std::lock_guard<std::mutex> lock(dbMutex);
    pqxx::work work(*connection);  // Создаем новую транзакцию

    try {
        std::string upsert_query = R"(
        WITH ins AS (
        INSERT INTO frequency (link_id, word_id, count) 
        VALUES ($1, $2, $3) 
        ON CONFLICT (link_id, word_id) 
        DO UPDATE SET count = EXCLUDED.count
        RETURNING *
        )
        SELECT * FROM ins;
        )";

        work.exec_params(upsert_query, link_id, word_id, frequency);
        work.commit();

    }
    catch (const std::exception& e) {
        work.abort();
        std::cerr << "Ошибка при добавлении частоты: " << e.what() << std::endl;
    }

}

std::vector<std::string> DB_Handle::get_query_result(const std::vector<std::string>& words) {
    std::lock_guard<std::mutex> lock(dbMutex);
    // Преобразуем в массив для передачи
    std::vector<std::string> res_;

    pqxx::work work(*connection);
    pqxx::array<std::string> word_array();
    try {
        pqxx::result result = work.exec_prepared("get_specific_word_frequency", words);

        for (const auto& row : result) {
            std::string url = row["url"].as<std::string>();
            res_.push_back(url);
        }

        work.commit();
    }
    catch (const std::exception& e) {
        std::cout << e.what() << std::endl;
    }

    return res_;
}