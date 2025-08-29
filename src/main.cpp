#include <iostream>
#include <thread>
#include <chrono>

#include <exec/static_thread_pool.hpp>
#include <stdexec/execution.hpp>

using namespace stdexec;

int main() {
    std::cout << "Hello stdexec!" << std::endl;

    // Create a static thread pool with 4 threads
    exec::static_thread_pool pool{4};

    // Get a scheduler from the thread pool
    auto scheduler = pool.get_scheduler();

    // Create a sender that sends the value 42
    auto sender = just(42);

    // Schedule the sender on the thread pool and add a continuation
    auto work = sender
        | then([](int value) {
            std::cout << "Received value: " << value << std::endl;
            std::cout << "Running on thread: "
                      << std::this_thread::get_id() << std::endl;
            return value * 2;
        })
        | then([](int value) {
            std::cout << "Doubled value: " << value << std::endl;
            std::cout << "Running on thread: "
                      << std::this_thread::get_id() << std::endl;
            return std::string("Result: ") + std::to_string(value);
        })
        | then([](const std::string& result) {
            std::cout << result << std::endl;
            std::cout << "Final thread: "
                      << std::this_thread::get_id() << std::endl;
        });

    // Schedule the work on the thread pool
    stdexec::sync_wait(std::move(work));

    std::cout << "Work completed!" << std::endl;

    return 0;
}
