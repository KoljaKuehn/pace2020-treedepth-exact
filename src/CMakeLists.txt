cmake_minimum_required (VERSION 3.23)
project (td_exact)

set(CMAKE_CXX_STANDARD 23)

add_compile_options(-O3 -flto -Wall -Wextra -Wshadow -g -Wfatal-errors -Wno-sign-compare -Wno-literal-suffix -fPIC -march=native)
file(GLOB SOURCES "*.cpp")
add_library(td_exact STATIC ${SOURCES})
target_include_directories(td_exact PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})