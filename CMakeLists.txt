cmake_minimum_required(VERSION 3.16)
project(application)

set(CMAKE_CXX_STANDARD 17)

cmake_policy(SET CMP0167 OLD)
set(CMAKE_AUTOMOC ON)

include(FindPkgConfig)

find_package(Qt6 REQUIRED COMPONENTS Widgets Svg SvgWidgets Network)
find_package(Boost REQUIRED COMPONENTS filesystem)
find_package(exiv2 REQUIRED)
include_directories(extern)

file(COPY assets DESTINATION build/assets)

pkg_check_modules(LIBHEIF REQUIRED IMPORTED_TARGET libheif)

set(CMAKE_C_COMPILER "clang")
set(CMAKE_CXX_COMPILER "clang++")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fms-extensions")
# set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -ftime-trace")


#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pg -g")
#set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -pg")

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_compile_options("-g")
add_compile_options("-O0")
add_compile_options(
    -Wall
    -Wextra
    -Wpedantic
    -Wconversion
    -Wsign-conversion
    -Wnon-virtual-dtor
    -Wold-style-cast
    -Wcast-align
    -Wunused
    -Woverloaded-virtual
    -Wformat=2
)

add_compile_definitions(ENABLE_BENCHMARKS=1)

file(GLOB_RECURSE SOURCES "src/*.cpp")

add_executable(application ${SOURCES})
target_precompile_headers(application PRIVATE src/pch.h)
link_directories("${CMAKE_SOURCE_DIR}/dlls")
target_link_libraries(
    application
    PRIVATE
    libboost_filesystem-mt
    Qt6::Widgets
    Qt6::Svg
    Qt6::SvgWidgets
    Qt6::Network
    exiv2
    PkgConfig::LIBHEIF
    ${CMAKE_SOURCE_DIR}/dlls/libqgeoview.dll.a
    #libqgeoview.dll
)
