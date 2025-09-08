// user2.cpp

#include "ksocket.hpp"
#include <fstream>
#include <vector>

int main(int argc, char* argv[]) {
    if (argc < 3) {
        std::cerr << "[user2] Usage: " << argv[0] << " <shmid> <output_filename>" << std::endl;
        return 1;
    }

    int shmid = std::stoi(argv[1]);
    std::string filename = argv[2];

    shm = attach_shared_memory(shmid);
    if (shm == nullptr) {
        std::cerr << "[user2] Failed to attach to shared memory." << std::endl;
        return 1;
    }

    int sockfd = k_socket();
    if (sockfd < 0) return 1;
    k_bind(sockfd, "127.0.0.1", PORT_USER2, "127.0.0.1", PORT_USER1);

    std::vector<char> buffer(MESSAGE_SIZE);
    
    // Wait for file size message
    int len = 0;
    while ((len = k_recvfrom(sockfd, buffer.data(), buffer.size())) <= 0) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    long expected_size = std::stol(std::string(buffer.data(), len));
    std::cout << "[user2] Expecting file size: " << expected_size << " bytes" << std::endl;

    std::ofstream file(filename, std::ios::binary);
    if (!file.is_open()) {
        std::cerr << "[user2] Failed to open output file: " << filename << std::endl;
        return 1;
    }

    long total_received = 0;
    while (true) {
        len = k_recvfrom(sockfd, buffer.data(), buffer.size());
        if (len > 0) {
            std::string received_str(buffer.data(), len);
            if (received_str == "EOF") {
                std::cout << "[user2] EOF marker received." << std::endl;
                break;
            }
            file.write(buffer.data(), len);
            total_received += len;
        } else {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
    }

    std::cout << "\n--- Reception Summary ---" << std::endl;
    std::cout << "Total Bytes Received: " << total_received << std::endl;
    if (total_received == expected_size) {
        std::cout << "File received successfully! ✅" << std::endl;
    } else {
        std::cout << "File size mismatch! Expected " << expected_size << ", got " << total_received << ". ❌" << std::endl;
    }
    std::cout << "-------------------------" << std::endl;

    k_close(sockfd);
    detach_shared_memory(shm);
    return 0;
}