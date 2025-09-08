// initksocket.cpp

#include "ksocket.hpp"
#include <vector>

// Use an atomic boolean for thread-safe signaling
static std::atomic<bool> keepRunning(true);

void intHandler(int signal) {
    std::cout << "\n[initksocket] Signal " << signal << " received. Shutting down..." << std::endl;
    keepRunning = false;
}

int main() {
    srand(time(nullptr));
    signal(SIGINT, intHandler);

    int shmid = init_shared_memory();
    if (shmid == -1) {
        std::cerr << "[initksocket] Failed to initialize shared memory" << std::endl;
        return 1;
    }

    // Launch threads using std::thread
    std::thread r_thread(thread_r, shmid);
    std::thread s_thread(thread_s, shmid);
    std::thread g_thread(thread_g, shmid);

    std::cout << "[initksocket] Shared memory created with ID: " << shmid << std::endl;
    std::cout << "[initksocket] Threads R, S, and G started" << std::endl;
    std::cout << "[initksocket] Run user applications with shmid=" << shmid << std::endl;
    std::cout << "[initksocket] Press Ctrl+C to exit" << std::endl;

    // Wait until signal is received
    while (keepRunning) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }

    // C++ doesn't have a direct pthread_cancel. Threads should be designed
    // to check keepRunning and exit gracefully. For this example, we'll
    // detach them and let the OS clean up upon exit.
    r_thread.detach();
    s_thread.detach();
    g_thread.detach();

    cleanup_shared_memory(shmid);
    std::cout << "[initksocket] Cleanup complete. Exiting." << std::endl;
    return 0;
}