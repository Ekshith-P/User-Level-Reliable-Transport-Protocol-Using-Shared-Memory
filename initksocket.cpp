#include "ksocket.h"
#include <iostream>
#include <thread>
#include <atomic>
#include <csignal>
#include <cstdlib>
#include <ctime>
#include <unistd.h> // for sleep()

// atomic flag for signal handling
static std::atomic<bool> keepRunning{true};

void intHandler(int) {
    keepRunning = false;
}

int main() {
    std::srand(std::time(nullptr));
    std::signal(SIGINT, intHandler);

    int shmid = init_shared_memory();
    if (shmid == -1) {
        std::cerr << "[initksocket] Failed to initialize shared memory\n";
        return 1;
    }

    ThreadArgs args{shmid};

    try {
        std::thread thread_r(thread_r, &args);
        std::thread thread_s(thread_s, &args);
        std::thread thread_g(thread_g, &args);

        std::cout << "[initksocket] Shared memory created with ID: " << shmid << "\n";
        std::cout << "[initksocket] Threads R, S, and G started\n";
        std::cout << "[initksocket] Run user1 and user2 with shmid=" << shmid << "\n";
        std::cout << "[initksocket] Press Ctrl+C to exit\n";

        while (keepRunning) {
            sleep(1);
        }

        std::cout << "[initksocket] Cleaning up...\n";

        // In C++, std::thread has no cancel, so we rely on threads exiting gracefully
        // If thread_r, thread_s, thread_g contain loops, they should check keepRunning too
        if (thread_r.joinable()) thread_r.join();
        if (thread_s.joinable()) thread_s.join();
        if (thread_g.joinable()) thread_g.join();

    } catch (const std::system_error& e) {
        std::cerr << "[initksocket] Thread creation failed: " << e.what() << "\n";
        cleanup_shared_memory(shmid);
        return 1;
    }

    cleanup_shared_memory(shmid);
    return 0;
}
