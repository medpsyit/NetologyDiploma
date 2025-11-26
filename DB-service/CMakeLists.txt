cmake_minimum_required(VERSION 3.20)
project(DBModule)
set(CMAKE_CXX_STANDARD 17)  # Или 14, или 20, если это необходимо

add_library(DB_module STATIC DB_service.cpp DB_service.h)

# target_include_directories(DB_module PRIVATE ${libpqxx_DIR})
# target_include_directories(DB_module PRIVATE ${Boost_INCLUDE_DIRS})

# find_package(PostgreSQL REQUIRED)
# find_package(libpqxx CONFIG REQUIRED)

target_link_libraries(DB_module PRIVATE PostgreSQL::PostgreSQL libpqxx::pqxx ${Boost_LIBRARIES} config_module)
