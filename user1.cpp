#include "ksocket.h"
#include <iostream>
#include <fstream>
#include <string>
#include <cstring>
#include <cstdlib>
#include <ctime>
#include <unistd.h>

constexpr size_t CHUNK_SIZE = 400;
constexpr int ACK_TIMEOUT = 5; // seconds

int main(int argc, char* argv[]) {
    if (argc < 3) {
        std::cerr << "[user1] Usage: " << argv[0] << " <shmid> <filename> [<drop_probability>]\n";
        return 1;
    }

    int shmid = std::atoi(argv[1]);
    const char* filename = argv[2];
    float drop_prob = P; // Default from ksocket.h
    if (argc >= 4) {
        drop_prob = std::atof(argv[3]);
    }

    std::cout << "[user1] Attaching to shmid=" << shmid
              << ", filename=" << filename
              << ", p=" << drop_prob << '\n';

    shm = attach_shared_memory(shmid);
    if (!shm) {
        std::cerr << "[user1] Failed to attach shared memory\n";
        return 1;
    }
    shm->drop_probability = drop_prob;

    std::ifstream file(filename, std::ios::binary);
    if (!file) {
        perror("[user1] Failed to open file");
        detach_shared_memory(shm);
        return 1;
    }

    int sockfd = k_socket();
    if (sockfd < 0 ||
        k_bind(sockfd, "127.0.0.1", PORT_USER1, "127.0.0.1", PORT_USER2) < 0) {
        std::cerr << "[user1] Socket creation or binding failed\n";
        file.close();
        detach_shared_memory(shm);
        return 1;
    }

    file.seekg(0, std::ios::end);
    std::streamoff file_size = file.tellg();
    file.seekg(0, std::ios::beg);

    char size_msg[32];
    std::snprintf(size_msg, sizeof(size_msg), "%ld", static_cast<long>(file_size));
    while (k_sendto(sockfd, size_msg, std::strlen(size_msg) + 1, 0) < 0) {
        sleep(1);
    }
    std::cout << "[user1] Sent file size: " << size_msg << '\n';

    char buffer[CHUNK_SIZE];
    size_t total_chunks = 0;
    size_t total_transmissions = 0;
    clock_t start = clock();

    while (file) {
        file.read(buffer, CHUNK_SIZE);
        std::streamsize bytesRead = file.gcount();
        if (bytesRead <= 0) break;

        total_chunks++;
        int retries = 0;
        while (k_sendto(sockfd, buffer, static_cast<size_t>(bytesRead), 0) < 0) {
            retries++;
            usleep(100000);
        }
        total_transmissions += (1 + retries);

        if (total_chunks % 100 == 0) {
            std::cout << "[user1] Sent " << total_chunks << " chunks\n";
        }
    }

    while (k_sendto(sockfd, "EOF", 4, 0) < 0) {
        usleep(100000);
    }

    clock_t end = clock();
    double time_taken = static_cast<double>(end - start) / CLOCKS_PER_SEC;

    file.close();

    char ack_buffer[MESSAGE_SIZE];
    time_t start_wait = time(nullptr);
    bool received_ack = false;

    while (time(nullptr) - start_wait < ACK_TIMEOUT) {
        int len = k_recvfrom(sockfd, ack_buffer, sizeof(ack_buffer), 0);
        if (len > 0 && std::strcmp(ack_buffer, "ACK_EOF") == 0) {
            received_ack = true;
            break;
        }
        usleep(100000);
    }

    if (!received_ack) {
        std::cout << "[user1] Warning: Did not receive ACK_EOF within "
                  << ACK_TIMEOUT << " seconds\n";
    } else {
        std::cout << "[user1] Received ACK_EOF\n";
    }

    std::cout << "[user1] Transmission complete: " << total_chunks
              << " chunks, " << total_transmissions << " transmissions\n";
    if (total_chunks > 0) {
        std::cout << "[user1] Avg transmissions per message: "
                  << static_cast<float>(total_transmissions) / total_chunks << '\n';
    }
    std::cout << "[user1] Time taken: " << time_taken << " seconds\n";
    if (time_taken > 0) {
        std::cout << "[user1] Throughput: "
                  << (file_size / 1024.0) / time_taken << " KB/s\n";
    }

    k_close(sockfd);
    detach_shared_memory(shm);
    return 0;
}
