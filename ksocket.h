#ifndef KSOCKET_HPP
#define KSOCKET_HPP

#include <array>
#include <cstdint>
#include <ctime>
#include <netinet/in.h> // sockaddr_in
#include <arpa/inet.h>  // inet_pton
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/socket.h>
#include <unistd.h>

// ---------- Constants ----------
constexpr int MAX_SOCKETS   = 2;
constexpr int MESSAGE_SIZE  = 512;
constexpr int WINDOW_SIZE   = 3;
constexpr int PORT_USER1    = 12345;
constexpr int PORT_USER2    = 54321;
constexpr int T_TIMEOUT_SEC = 5;     // T
constexpr float P_DROP      = 0.1f;  // P

// Error codes (kept as-is for compatibility)
constexpr int ENOSPACE    = -1;
constexpr int ENOTBOUND   = -2;
constexpr int ENOMESSAGE  = -3;

// ---------- Message Types ----------
enum : std::uint8_t {
    MSG_TYPE_DATA = 0,
    MSG_TYPE_ACK  = 1
};

// ---------- Data Structures ----------
struct KTP_Message {
    std::uint8_t seq_num;
    std::uint8_t type;
    std::array<char, MESSAGE_SIZE> payload{};
};

struct SenderWindow {
    std::array<KTP_Message, WINDOW_SIZE> messages{};
    int base{0};
    int next_seq{0};
    std::array<std::time_t, WINDOW_SIZE> timestamps{};
    int size{0};
};

struct ReceiverWindow {
    std::array<KTP_Message, WINDOW_SIZE> messages{};
    int base{0};
    int size{0};
    int rwnd{WINDOW_SIZE};
};

struct KTP_Socket {
    int  is_free{1};
    pid_t process_id{0};
    int  sock_id{-1};
    sockaddr_in src_addr{};
    sockaddr_in dest_addr{};

    std::array<KTP_Message, WINDOW_SIZE> send_buffer{};
    std::array<KTP_Message, WINDOW_SIZE> recv_buffer{};
    int send_count{0};
    int recv_count{0};

    SenderWindow  swnd{};
    ReceiverWindow rwnd{};
};

struct SharedMemory {
    std::array<KTP_Socket, MAX_SOCKETS> sockets{};
    float drop_probability{P_DROP};
};

// Extern shared memory pointer (defined in a .cpp)
extern SharedMemory* shm;

// ---------- API (same signatures for easy porting) ----------
int  init_shared_memory();
SharedMemory* attach_shared_memory(int shmid);
void detach_shared_memory(SharedMemory* shm_ptr);
void cleanup_shared_memory(int shmid);

int  k_socket();
int  k_bind(int sockfd, const char* src_ip, int src_port,
            const char* dest_ip, int dest_port);
int  k_sendto(int sockfd, const void* buf, size_t len, int flags);
int  k_recvfrom(int sockfd, void* buf, size_t len, int flags);
int  k_close(int sockfd);

// Threads (C++ implementations can wrap these)
void* thread_r(void* arg);
void* thread_s(void* arg);
void* thread_g(void* arg);

struct ThreadArgs {
    int shmid;
};

// Keep same signature for drop probability hook
int dropMessage(float probability);

#endif // KSOCKET_HPP
