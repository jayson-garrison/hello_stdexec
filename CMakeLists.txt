cmake_minimum_required(VERSION 3.25)
project(hello_stdexec VERSION 1.0.0 LANGUAGES CXX)

# Set C++ standard to 20
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Enable FetchContent
include(FetchContent)

# Fetch NVIDIA's stdexec library
FetchContent_Declare(
    stdexec
    GIT_REPOSITORY https://github.com/NVIDIA/stdexec.git
    GIT_TAG main
)

# Make stdexec available
FetchContent_MakeAvailable(stdexec)

# Create executable target
add_executable(hello_stdexec
    src/main.cpp
)

# Link stdexec library
target_link_libraries(hello_stdexec
    PRIVATE
        STDEXEC::stdexec
)

# Set include directories
target_include_directories(hello_stdexec
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

# Enable warnings
if(MSVC)
    target_compile_options(hello_stdexec PRIVATE /W4)
else()
    target_compile_options(hello_stdexec PRIVATE -Wall -Wextra -Wpedantic)
endif()

# Set compile definitions for stdexec
target_compile_definitions(hello_stdexec
    PRIVATE
        STDEXEC_ENABLE_EXTRA_TYPE_CHECKING
)
