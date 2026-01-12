cmake_minimum_required(VERSION 3.10)
project(zipcontainer VERSION 0.0.1 DESCRIPTION "Zip file as uncompressed container library" LANGUAGES C CXX)
enable_testing()

if (NOT DEFINED CMAKE_CXX_STANDARD)
    set(CMAKE_CXX_STANDARD 20)
endif()
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_CTEST_ARGUMENTS "--output-on-failure")
set(SRC_ROOT ${PROJECT_SOURCE_DIR})

add_library(zipcontainer STATIC ${PROJECT_SOURCE_DIR}/zipc.cpp ${PROJECT_SOURCE_DIR}/zipc.h)
target_include_directories(zipcontainer PUBLIC ${PROJECT_SOURCE_DIR})
target_compile_options(zipcontainer PRIVATE -fPIC -Wall)

add_executable(zipctest tests/zipctest.cpp ${PROJECT_SOURCE_DIR}/zipc.cpp ${PROJECT_SOURCE_DIR}/zipc.h)
target_include_directories(zipctest PUBLIC ${PROJECT_SOURCE_DIR})
target_compile_options(zipctest PRIVATE -Wall)
add_test(NAME zipctest COMMAND ${CMAKE_CURRENT_BINARY_DIR}/zipctest)
