cmake_minimum_required(VERSION 3.16.3 FATAL_ERROR)

project(SearchServer)

set(SOURCES 
    src/main.cpp
)

set(HEADERS
    src/concurrent_map.h
    src/log_duration.h
    src/paginator.h
    src/test_example_functions.h
)

set(PAIRS
    src/document.h src/document.cpp
    src/process_queries.h src/process_queries.cpp
    src/read_input_functions.h src/read_input_functions.cpp
    src/remove_duplicates.h src/remove_duplicates.cpp
    src/request_queue.h src/request_queue.cpp
    src/search_server.h src/search_server.cpp
    src/string_processing.h src/string_processing.h
)

add_executable(server ${SOURCES} ${HEADERS} ${PAIRS})

set(CXX_COVERAGE_COMPILE_FLAGS "-std=c++17 -Wall -Werror -g")
set(CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS} ${CXX_COVERAGE_COMPILE_FLAGS}")

set_target_properties(
    server PROPERTIES
    CXX_STANDART 17
    CXX_STANDART_REQUIRED ON
)