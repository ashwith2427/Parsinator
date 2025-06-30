cmake_minimum_required(VERSION 3.14)
project(tests)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
include(FetchContent)
FetchContent_Declare(
  googletest
  URL https://github.com/google/googletest/archive/refs/heads/main.zip
)
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)
add_executable(tests parser_test.cpp)
target_link_libraries(tests gtest gtest_main)
target_include_directories(tests PRIVATE ${CMAKE_SOURCE_DIR})
add_test(NAME parser_test COMMAND tests)
