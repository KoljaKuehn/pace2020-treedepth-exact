cmake_minimum_required (VERSION 3.23)
project (td_exact)

set(CMAKE_CXX_STANDARD 23)

add_compile_options(-O3 -flto -Wall -Wextra -Wshadow -g -Wfatal-errors -Wno-sign-compare -Wno-literal-suffix -fPIC -march=native)
add_library(td_exact STATIC
        graph.cpp
        io.cpp
        preprocessor.cpp
        solve.cpp
        utils.cpp
)
target_include_directories(td_exact PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

if(CMAKE_CURRENT_SOURCE_DIR STREQUAL CMAKE_SOURCE_DIR)
  add_executable(td_exact_standalone main.cpp)
  target_link_libraries(td_exact_standalone PRIVATE td_exact)
endif()