cmake_minimum_required(VERSION 3.24)

set(CMAKE_C_STANDARD   17)
set(CMAKE_CXX_STANDARD 20)

project(CQLite)
add_library(CQLite INTERFACE)
target_link_libraries(CQLite INTERFACE sqlite3)
target_include_directories(CQLite INTERFACE ${CMAKE_CURRENT_LIST_DIR})
