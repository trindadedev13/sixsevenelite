cmake_minimum_required(VERSION 3.25)

project(botapi
    LANGUAGES CXX)

set(CMAKE_CXX_STANDARD            26)
set(CMAKE_CXX_STANDARD_REQUIRED   ON)
set(CMAKE_CXX_EXTENSIONS          OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(CURL          REQUIRED)
find_package(nlohmann_json REQUIRED)

add_library(botapi
    src/api.cpp)

target_include_directories(botapi
    PUBLIC include)

target_link_libraries(botapi
    PUBLIC
        CURL::libcurl
        nlohmann_json::nlohmann_json)

add_executable(sixsevenelite
    src/main.cpp
    src/run_code.cpp)

target_link_libraries(sixsevenelite
    PRIVATE
    botapi)
