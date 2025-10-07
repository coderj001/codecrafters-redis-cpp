cmake_minimum_required(VERSION 3.13)

project(redis-starter-cpp)

file(GLOB SOURCE_FILES src/*.cpp)

file(GLOB LIB_SOURCE_FILES src/handle_command.cpp src/kv_store.cpp src/resp_parser.cpp)

add_library(redis-lib ${LIB_SOURCE_FILES})


set(CMAKE_CXX_STANDARD 23) # Enable the C++23 standard
set(THREADS_PREFER_PTHREAD_FLAG ON)

# Add debug configuration
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug)
endif()

# Add debug flags
set(CMAKE_CXX_FLAGS_DEBUG "-g -O0 -DDEBUG")
set(CMAKE_CXX_FLAGS_RELEASE "-O3 -DNDEBUG")

find_package(Threads REQUIRED)
find_package(asio CONFIG REQUIRED)
find_package(GTest REQUIRED)

add_executable(redis-server src/Server.cpp)

target_link_libraries(redis-server PRIVATE redis-lib asio::asio Threads::Threads)

# Add compile options for better debugging
target_compile_options(redis-server PRIVATE
    $<$<CONFIG:Debug>:-g3 -ggdb -fno-omit-frame-pointer>
)

# Unit tests
file(GLOB TEST_FILES src/tests/*.cpp)
add_executable(unit_tests ${TEST_FILES})
target_link_libraries(unit_tests PRIVATE GTest::gtest_main redis-lib)

enable_testing()
add_test(NAME StoreValueTest COMMAND unit_tests)
add_test(NAME KvStoreTest COMMAND unit_tests)
