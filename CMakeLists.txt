cmake_minimum_required(VERSION 3.25)
project(graphs VERSION 0.0.14)

option(ENABLE_ADDRSAN "Enable the address sanitizer" OFF)
option(ENABLE_UBSAN "Enable the ub sanitizer" OFF)
option(ENABLE_TSAN "Enable the thread data race sanitizer" OFF)

message(${CMAKE_CXX_COMPILER_ID})

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_subdirectory(lib/BLT-With-Graphics-Template SYSTEM)

include_directories(include/)
file(GLOB_RECURSE PROJECT_BUILD_FILES "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp")

add_executable(graphs ${PROJECT_BUILD_FILES})

target_compile_options(graphs PRIVATE -Wall -Wextra -Wno-comment)
target_link_options(graphs PRIVATE -Wall -Wextra -Wno-comment)

target_link_libraries(graphs PUBLIC BLT_WITH_GRAPHICS)

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
    set_target_properties(graphs PROPERTIES LINK_FLAGS "-sMAX_WEBGL_VERSION=2 -sASSERTIONS=1 -sUSE_GLFW=3")
endif ()