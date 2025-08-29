# hello_stdexec

A modern C++20 project demonstrating NVIDIA's stdexec library with a simple sender example using `just(42)` and scheduling on a static thread pool.

## Features

- **C++20 Standard**: Uses modern C++ features and concepts
- **stdexec Library**: NVIDIA's reference implementation of the C++ standard executors
- **Static Thread Pool**: Demonstrates scheduling work on a thread pool
- **Sender/Receiver Pattern**: Shows the core stdexec programming model
- **Modern CMake**: Uses FetchContent to automatically download dependencies

## Project Structure

```
hello_stdexec/
├── CMakeLists.txt          # CMake configuration with stdexec dependency
├── include/               # Header files (if needed)
├── src/
│   └── main.cpp           # Main application with stdexec example
├── README.md              # This file
└── .git/                  # Git repository
```

## Prerequisites

- CMake 3.25 or higher (we use snap-installed CMake 4.1.1)
- C++20 compatible compiler (GCC 10+, Clang 12+, MSVC 2022+)
- Git (for fetching stdexec dependency)

## Building

### Build Instructions

1. Create a build directory:
   ```bash
   mkdir build
   cd build
   ```

2. Configure the project:
   ```bash
   cmake ..
   ```
   This will automatically download NVIDIA's stdexec library.

3. Build the project:
   ```bash
   cmake --build .
   ```

4. Run the application:
   ```bash
   ./hello_stdexec
   ```

### Alternative Build Commands

You can also build and run in one step:
```bash
mkdir build && cd build && cmake .. && cmake --build . && ./hello_stdexec
```

### Using System CMake (if available)

If you have CMake 3.25+ installed system-wide, you can use:
```bash
cmake .. && cmake --build .
```

## What the Example Does

The example demonstrates:

1. **Creating a Static Thread Pool**: `exec::static_thread_pool pool{4};`
2. **Getting a Scheduler**: `auto scheduler = pool.get_scheduler();`
3. **Creating a Simple Sender**: `auto sender = just(42);`
4. **Building a Chain of Operations**:
   - `then()` - Transform values
5. **Waiting for Completion**: `sync_wait()`

### Expected Output

```
Hello stdexec!
Received value: 42
Running on thread: 140234567890123
Doubled value: 84
Running on thread: 140234567890124
Result: 84
Final thread: 140234567890125
Work completed!
```

The thread IDs will vary, but you'll see that different stages of the pipeline run on different threads from the thread pool.

## stdexec Concepts Demonstrated

- **Senders**: Objects that can send values (e.g., `just(42)`)
- **Receivers**: Objects that can receive values and handle completion
- **Schedulers**: Objects that determine where and when work executes
- **Algorithms**: Functions that transform senders (e.g., `then`)
- **Thread Pools**: Execution contexts for concurrent work

## Modern C++ Practices Used

- **C++20 Features**: Concepts, ranges, coroutines support
- **RAII**: Proper resource management
- **Exception Safety**: No-throw operations where possible
- **CMake Best Practices**: Target-based builds, automatic dependency management
- **Namespace Usage**: Clean separation of concerns

## Dependencies

- **stdexec**: Automatically fetched from https://github.com/NVIDIA/stdexec.git
- **C++ Standard Library**: Uses `<iostream>`, `<thread>`, `<string>`

## License

See LICENSE file for details.
