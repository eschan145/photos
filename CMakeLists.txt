cmake_minimum_required(VERSION 3.16)
project(application)

set(CMAKE_CXX_STANDARD 17)

find_package(Qt6 REQUIRED COMPONENTS Widgets)
find_package(Boost REQUIRED COMPONENTS filesystem)
find_package(exiv2 REQUIRED)

set(CMAKE_C_COMPILER "clang")
set(CMAKE_CXX_COMPILER "clang++")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fms-extensions")
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_compile_options("-g")
add_compile_options("-O0")

file(GLOB_RECURSE SOURCES "src/*.cpp")

add_executable(application ${SOURCES})
target_link_libraries(application PRIVATE libboost_filesystem-mt Qt6::Widgets exiv2)
