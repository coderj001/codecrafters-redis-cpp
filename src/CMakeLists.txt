# src/CMakeLists.txt

# Gather all .cpp files in the src directory
file(GLOB_RECURSE SOURCE_FILES "*.cpp")

# Create a library with the core logic
add_library(redis_lib ${SOURCE_FILES})

# Make the 'include' directory available to other targets that link against redis_lib
target_include_directories(redis_lib PUBLIC include)

# Link the library to its dependencies
target_link_libraries(redis_lib PRIVATE asio::asio Threads::Threads)

# Create the server executable from Server.cpp
add_executable(server Server.cpp)

# Link the server executable against our library
target_link_libraries(server PRIVATE redis_lib)

# Add compile options for better debugging
target_compile_options(server PRIVATE 
    $<$<CONFIG:Debug>:-g3 -ggdb -fno-omit-frame-pointer>
)
