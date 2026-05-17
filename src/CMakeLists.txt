cmake_minimum_required (VERSION 3.23)
project (td_exact)

set(CMAKE_CXX_STANDARD 23)

add_compile_options(-flto -Wall -Wextra -Wshadow -Wfatal-errors -Wno-sign-compare -Wno-literal-suffix -march=native)
add_library(td_exact STATIC
        graph.cpp
        io.cpp
        ms_solve.cpp
        preprocessor.cpp
        solve.cpp
)
target_include_directories(td_exact PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

if(CMAKE_CURRENT_SOURCE_DIR STREQUAL CMAKE_SOURCE_DIR)
  add_executable(td_exact_standalone main.cpp)
  target_link_libraries(td_exact_standalone PRIVATE td_exact)
endif()