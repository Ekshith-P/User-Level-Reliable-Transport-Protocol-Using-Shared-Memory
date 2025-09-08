// ksocket.hpp

#ifndef KSOCKET_HPP
#define KSOCKET_HPP

// C++ Standard Libraries
#include <iostream>
#include <string>
#include <vector>
#include <chrono>
#include <thread>
#include <atomic>

// C System Libraries (for low-level socket and IPC)
#include <unistd.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <cstring>
#include <csignal>
#include <cstdlib>
#include <cerrno>

// --- Constants ---
constexpr int MAX_SOCKETS = 2;
constexpr int WINDOW_SIZE = 10;
constexpr int MESSAGE_SIZE = 512;
constexpr int T = 2;       // Timeout in seconds for retransmission
constexpr double P = 0.1;  // Default drop probability

// --- Ports for User Applications ---
constexpr int PORT_USER1 = 8080;
constexpr int PORT_USER2 = 8081;

// --- Error Codes ---
constexpr int ENOSPACE = -1;
constexpr int ENOTBOUND = -2;
constexpr int ENOMESSAGE = -3;

// --- Message Types ---
enum MessageType { MSG_TYPE_DATA, MSG_TYPE_ACK };

// --- Data Structures (Must be POD for shared memory) ---

struct KTP_Message {
    int type;
    int seq_num;
    char payload[MESSAGE_SIZE];
};

struct SendWindow {
    int base;
    int next_seq;
    int size;
    KTP_Message messages[WINDOW_SIZE];
    time_t timestamps[WINDOW_SIZE];
};

struct ReceiveWindow {
    int base;
    int size;
    int rwnd;
    KTP_Message messages[WINDOW_SIZE];
};

struct KTP_Socket {
    int is_free;
    pid_t process_id;
    int sock_id;
    struct sockaddr_in src_addr;
    struct sockaddr_in dest_addr;

    KTP_Message send_buffer[WINDOW_SIZE];
    int send_count;
    KTP_Message recv_buffer[WINDOW_SIZE];
    int recv_count;

    SendWindow swnd;
    ReceiveWindow rwnd;
};

struct SharedMemory {
    KTP_Socket sockets[MAX_SOCKETS];
    double drop_probability;
};

// --- Global Shared Memory Pointer ---
extern SharedMemory *shm;

// --- Function Prototypes ---

// Shared Memory Management
int init_shared_memory();
SharedMemory* attach_shared_memory(int shmid);
void detach_shared_memory(SharedMemory* shm_ptr);
void cleanup_shared_memory(int shmid);

// KTP Socket API
int k_socket();
int k_bind(int sockfd, const std::string& src_ip, int src_port, const std::string& dest_ip, int dest_port);
int k_sendto(int sockfd, const void* buf, size_t len);
int k_recvfrom(int sockfd, void* buf, size_t len);
int k_close(int sockfd);

// Daemon Threads
void thread_r(int shmid); // Receiver thread
void thread_s(int shmid); // Sender thread
void thread_g(int shmid); // Garbage collector thread

// Utility
bool dropMessage(double probability);

#endif // KSOCKET_HPP