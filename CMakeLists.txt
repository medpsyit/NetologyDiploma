cmake_minimum_required(VERSION 3.20)

project(CppDiplomProject)

set(CMAKE_PREFIX_PATH "F:\\Boost\\boost_1_89_0;F:\\Boost\\install" CACHE STRING "Boost install prefix" FORCE)

set(CMAKE_TOOLCHAIN_FILE "C:/vcpkg/scripts/buildsystems/vcpkg.cmake" CACHE STRING "Vcpkg toolchain file")
set(CMAKE_PREFIX_PATH "C:/vcpkg/installed/x64-windows" CACHE STRING "Path to vcpkg")


set(Boost_USE_STATIC_LIBS OFF)  # Использовать динамические библиотеки

# TODO: Define by your own needs:
set(OPENSSL_ROOT_DIR "C:\\Program Files\\OpenSSL-Win64")

set(BOOST_ROOT "F:\\Boost\\boost_1_89_0")
set(Boost_LIBRARY_DIR "F:/Boost/install/lib")

set(libpqxx_DIR "C:/vcpkg/installed/x64-windows/share/libpqxx")

# set(libpqxx_DIR "C:/vcpkg/installed/x64-windows/include")
# set(libpqxx_LIB "C:/vcpkg/installed/x64-windows/debug/lib/pqxx.lib")

set(BOOST_MIN_VERSION "1.70.0")

# TODO: If you are running on Windows below 10, change this:

# add_compile_definitions(_WIN32_WINNT=0x0A01)


# Boost:
find_package(Boost ${BOOST_MIN_VERSION} REQUIRED locale)

if(NOT Boost_FOUND)
    message(FATAL_ERROR "Could not find Boost")
endif()

# Вывод информации для отладки
message(STATUS "Boost_INCLUDE_DIRS: ${Boost_INCLUDE_DIRS}")
message(STATUS "Boost_LIBRARIES: ${Boost_LIBRARIES}")

# PQXX
set(PostgreSQL_ADDITIONAL_VERSIONS "15")

# add_subdirectory(${libpqxx_DIR} libpqxx-build) 

find_package(PostgreSQL REQUIRED)
find_package(libpqxx CONFIG REQUIRED)

message(STATUS "PostgreSQL_INCLUDE_DIRS: ${PostgreSQL_INCLUDE_DIRS}")
message(STATUS "PostgreSQL_LIBRARIES: ${PostgreSQL_LIBRARIES}")

message(STATUS "libpqxx_INCLUDE_DIRS: ${libpqxx_INCLUDE_DIRS}")
message(STATUS "libpqxx_LIBRARIES: ${libpqxx_LIBRARIES}")

# OpenSSL
find_package(OpenSSL REQUIRED)


add_subdirectory(Http-client)

add_subdirectory(Http-server)

add_subdirectory(Config)

add_subdirectory(DB-service)

file(COPY Config/config.ini DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
