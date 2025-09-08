// user1.cpp

#include "ksocket.hpp"
#include <fstream>
#include <vector>

constexpr int CHUNK_SIZE = 400;

int main(int argc, char* argv[]) {
    if (argc < 3) {
        std::cerr << "[user1] Usage: " << argv[0] << " <shmid> <filename> [<drop_probability>]" << std::endl;
        return 1;
    }

    int shmid = std::stoi(argv[1]);
    std::string filename = argv[2];
    double drop_prob = (argc >= 4) ? std::stod(argv[3]) : P;

    shm = attach_shared_memory(shmid);
    if (shm == nullptr) {
        std::cerr << "[user1] Failed to attach to shared memory." << std::endl;
        return 1;
    }
    shm->drop_probability = drop_prob;

    std::ifstream file(filename, std::ios::binary | std::ios::ate);
    if (!file.is_open()) {
        std::cerr << "[user1] Failed to open file: " << filename << std::endl;
        detach_shared_memory(shm);
        return 1;
    }
    std::streamsize file_size = file.tellg();
    file.seekg(0, std::ios::beg);

    int sockfd = k_socket();
    if (sockfd < 0) return 1;
    k_bind(sockfd, "127.0.0.1", PORT_USER1, "127.0.0.1", PORT_USER2);

    // Send file size first
    std::string size_msg = std::to_string(file_size);
    while (k_sendto(sockfd, size_msg.c_str(), size_msg.length() + 1) == ENOSPACE) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    std::vector<char> buffer(CHUNK_SIZE);
    auto start_time = std::chrono::high_resolution_clock::now();

    while (file.read(buffer.data(), CHUNK_SIZE) || file.gcount() > 0) {
        while (k_sendto(sockfd, buffer.data(), file.gcount()) == ENOSPACE) {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
    }

    // Send EOF marker
    while (k_sendto(sockfd, "EOF", 4) == ENOSPACE) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    // Wait for all data to be acknowledged
    while (shm->sockets[sockfd].swnd.size > 0 || shm->sockets[sockfd].send_count > 0) {
        std::cout << "[user1] Waiting for final ACKs... (window size: " << shm->sockets[sockfd].swnd.size << ")" << std::endl;
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
    
    auto end_time = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> elapsed = end_time - start_time;

    std::cout << "\n--- Transmission Summary ---" << std::endl;
    std::cout << "File Size: " << file_size << " bytes" << std::endl;
    std::cout << "Time Taken: " << elapsed.count() << " seconds" << std::endl;
    if (elapsed.count() > 0) {
        double throughput = (static_cast<double>(file_size) / 1024.0) / elapsed.count();
        std::cout << "Throughput: " << throughput << " KB/s" << std::endl;
    }
    std::cout << "--------------------------" << std::endl;

    k_close(sockfd);
    detach_shared_memory(shm);
    return 0;
}