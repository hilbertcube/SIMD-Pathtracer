cmake_minimum_required(VERSION 3.18)
project(Ray_tracer LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)

# Default to an optimized build: a Debug build renders about 9x slower
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

# Compiler Info
message(STATUS "C Compiler: ${CMAKE_C_COMPILER}")
message(STATUS "C++ Compiler: ${CMAKE_CXX_COMPILER}")

# Flags
if(CMAKE_BUILD_TYPE STREQUAL "Release")
    set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} -O3")
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O3")
    set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} -march=native -DNDEBUG")
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -march=native -DNDEBUG")
endif()

# Find and link OpenMP
find_package(OpenMP REQUIRED)
if(OpenMP_CXX_FOUND)
    message(STATUS "OpenMP found")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${OpenMP_CXX_FLAGS}")
endif()

# `cmake --build build --target format` applies .clang-format to the project's own sources
find_program(CLANG_FORMAT clang-format)
if(CLANG_FORMAT)
    file(GLOB_RECURSE FORMAT_SOURCES CONFIGURE_DEPENDS
        "${CMAKE_SOURCE_DIR}/src/*.cpp" "${CMAKE_SOURCE_DIR}/src/*.h")
    list(FILTER FORMAT_SOURCES EXCLUDE REGEX "/src/external/")
    add_custom_target(format
        COMMAND ${CLANG_FORMAT} -i ${FORMAT_SOURCES}
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        COMMENT "Running clang-format")
endif()

file(GLOB C_SOURCES "src/*.c")
file(GLOB CPP_SOURCES "src/*.cpp")

# Compiled into every executable: the scenes (one file each), and the third-party single-header
# libraries, which are compiled once in src/rt/external_impl.cpp
file(GLOB_RECURSE SHARED_SOURCES CONFIGURE_DEPENDS "src/scenes/*.cpp" "src/rt/*.cpp")

foreach(SOURCE_FILE ${C_SOURCES} ${CPP_SOURCES})
    get_filename_component(TARGET_NAME ${SOURCE_FILE} NAME_WE)
    add_executable(${TARGET_NAME} ${SOURCE_FILE} ${SHARED_SOURCES})
    # Includes are written from src/, e.g. #include "rt/math/vector.h"
    target_include_directories(${TARGET_NAME} PRIVATE "${CMAKE_SOURCE_DIR}/src")
    target_link_libraries(${TARGET_NAME} PRIVATE
        OpenMP::OpenMP_CXX
    )
endforeach()
