cmake_minimum_required(VERSION 3.5)

# Project name and language
project(test_atomics LANGUAGES C)

# Enforce C11 standard compliance
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

# Find and link threads (pthreads on Unix; no extra lib on Windows)
set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

# Build the executable from main.c
add_executable(test_atomics src/main.c src/tinycthread.c)

# Link against Threads::Threads (adds -pthread or equivalent as needed)
target_link_libraries(test_atomics PRIVATE Threads::Threads)
