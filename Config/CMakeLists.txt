cmake_minimum_required(VERSION 3.20)
project(ConfigModule)

add_library(config_module STATIC config.cpp config.h)

target_include_directories(config_module PRIVATE ${Boost_INCLUDE_DIRS})

target_link_libraries(config_module ${Boost_LIBRARIES})
