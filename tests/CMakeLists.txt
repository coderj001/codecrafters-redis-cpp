# tests/CMakeLists.txt

# Find the GTest package
find_package(GTest CONFIG REQUIRED)
include(GoogleTest)

# Enable testing for this directory and its subdirectories
enable_testing()

# Create the test executable
add_executable(unit_tests store_value_test.cpp)

# Link the test executable against our main library and GTest
target_link_libraries(unit_tests PRIVATE redis_lib GTest::gtest_main)

# Discover and add tests to the CTest test runner
gtest_discover_tests(unit_tests)
