#include "ksocket.h"
#include <iostream>
#include <fstream>
#include <cstring>
#include <cstdlib>
#include <ctime>
#include <unistd.h>

constexpr size_t CHUNK_SIZE = 400;

int main(int argc, char* argv[]) {
    if (argc < 3) {
        std::cerr << "[user2] Usage: " << argv[0] << " <shmid> <output_filename>\n";
        return 1;
    }

    int shmid = std::atoi(argv[1]);
    const char* filename = argv[2];

    std::cout << "[user2] Attaching to shmid=" << shmid
              << ", output=" << filename << '\n';

    shm = attach_shared_memory(shmid);
    if (!shm) {
        return 1;
    }

    int sockfd = k_socket();
    if (sockfd < 0 ||
        k_bind(sockfd, "127.0.0.1", PORT_USER2, "127.0.0.1", PORT_USER1) < 0) {
        detach_shared_memory(shm);
        return 1;
    }

    char buffer[MESSAGE_SIZE];
    // wait for first message (file size)
    while (true) {
        int len = k_recvfrom(sockfd, buffer, sizeof(buffer), 0);
        if (len > 0) break;
        sleep(1);
    }

    long expected_size = std::atol(buffer);
    std::cout << "[user2] Expected file size: " << expected_size << " bytes\n";

    std::ofstream file(filename, std::ios::binary);
    if (!file) {
        perror("[user2] Failed to open output file");
        k_close(sockfd);
        detach_shared_memory(shm);
        return 1;
    }

    long total_received = 0;
    int chunks_received = 0;
    clock_t start = clock();

    while (total_received < expected_size) {
        int len = k_recvfrom(sockfd, buffer, sizeof(buffer), 0);
        if (len > 0) {
            if (std::strcmp(buffer, "EOF") == 0) {
                break;
            }
            file.write(buffer, len);
            total_received += len;
            chunks_received++;
            if (chunks_received % 100 == 0) {
                std::cout << "[user2] Received " << total_received
                          << " bytes (" << chunks_received << " chunks)\n";
            }
        }
        usleep(1000);
    }

    file.close();
    clock_t end = clock();
    double time_taken = static_cast<double>(end - start) / CLOCKS_PER_SEC;

    std::cout << "[user2] Received: " << total_received
              << " bytes in " << chunks_received << " chunks\n";
    std::cout << "[user2] Time taken: " << time_taken << " seconds\n";
    if (time_taken > 0) {
        std::cout << "[user2] Throughput: "
                  << (total_received / 1024.0) / time_taken << " KB/s\n";
    }

    int retries = 0;
    while (k_sendto(sockfd, "ACK_EOF", 8, 0) < 0 && retries < 10) {
        retries++;
        sleep(1);
    }
    if (retries >= 10) {
        std::cout << "[user2] Warning: Failed to send ACK_EOF after "
                  << retries << " retries\n";
    } else {
        std::cout << "[user2] Sent ACK_EOF\n";
    }

    k_close(sockfd);
    detach_shared_memory(shm);
    return 0;
}
