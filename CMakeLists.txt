cmake_minimum_required(VERSION 3.25)
project(graphs)

option(ENABLE_ADDRSAN "Enable the address sanitizer" OFF)
option(ENABLE_UBSAN "Enable the ub sanitizer" OFF)
option(ENABLE_TSAN "Enable the thread data race sanitizer" OFF)

set(CMAKE_CXX_STANDARD 17)

include_directories(include/)
file(GLOB_RECURSE PROJECT_BUILD_FILES "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp")

add_executable(graphs ${PROJECT_BUILD_FILES})

target_compile_options(graphs PRIVATE -Wall -Werror -Wpedantic -Wno-comment)
target_link_options(graphs PRIVATE -Wall -Werror -Wpedantic -Wno-comment)

if (${ENABLE_ADDRSAN} MATCHES ON)
    target_compile_options(graphs PRIVATE -fsanitize=address)
    target_link_options(graphs PRIVATE -fsanitize=address)
endif ()

if (${ENABLE_UBSAN} MATCHES ON)
    target_compile_options(graphs PRIVATE -fsanitize=undefined)
    target_link_options(graphs PRIVATE -fsanitize=undefined)
endif ()

if (${ENABLE_TSAN} MATCHES ON)
    target_compile_options(graphs PRIVATE -fsanitize=thread)
    target_link_options(graphs PRIVATE -fsanitize=thread)
endif ()