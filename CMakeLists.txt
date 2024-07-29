cmake_minimum_required(VERSION 3.25)
project(graphs VERSION 0.1.5)

option(ENABLE_ADDRSAN "Enable the address sanitizer" OFF)
option(ENABLE_UBSAN "Enable the ub sanitizer" OFF)
option(ENABLE_TSAN "Enable the thread data race sanitizer" OFF)

message(${CMAKE_CXX_COMPILER_ID})

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_subdirectory(lib/BLT-With-Graphics-Template SYSTEM)
set(JSON_BuildTests OFF CACHE INTERNAL "")
add_subdirectory(lib/json)

include_directories(include/)
file(GLOB_RECURSE PROJECT_BUILD_FILES "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp")

add_executable(graphs ${PROJECT_BUILD_FILES})

target_compile_options(graphs PRIVATE -Wall -Wextra -Wno-comment)
target_link_options(graphs PRIVATE -Wall -Wextra -Wno-comment)

target_link_libraries(graphs PUBLIC BLT_WITH_GRAPHICS)
target_link_libraries(graphs PRIVATE nlohmann_json::nlohmann_json)

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

if (EMSCRIPTEN)
    set(BLT_PRELOAD_PATH "../res@../res")
    include(lib/BLT-With-Graphics-Template/cmake/link_flags.cmake)
endif ()