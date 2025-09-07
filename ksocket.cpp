#include "ksocket.h"

#include <iostream>
#include <cstring>
#include <csignal>
#include <cerrno>
#include <pthread.h>
#include <sys/time.h>
#include <sys/select.h>
#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/ipc.h>
#include <sys/shm.h>

SharedMemory* shm = nullptr;

int dropMessage(float probability) {
    double r = static_cast<double>(rand()) / RAND_MAX;
    bool dropped = (r < probability);
    if (dropped) {
        std::cout << "[dropMessage] Packet dropped (probability=" << probability << ")\n";
    }
    return dropped ? 1 : 0;
}

int init_shared_memory() {
    std::cout << "[init_shared_memory] Initializing shared memory...\n";
    int shmid = shmget(IPC_PRIVATE, sizeof(SharedMemory), 0666 | IPC_CREAT);
    if (shmid == -1) {
        perror("[init_shared_memory] shmget failed");
        return -1;
    }

    shm = attach_shared_memory(shmid);
    if (!shm) return -1;

    for (int i = 0; i < MAX_SOCKETS; ++i) {
        shm->sockets[i].is_free = 1;
        shm->sockets[i].process_id = 0;
        shm->sockets[i].sock_id = -1;
        shm->sockets[i].send_count = 0;
        shm->sockets[i].recv_count = 0;
        shm->sockets[i].swnd.base = 0;
        shm->sockets[i].swnd.next_seq = 0;
        shm->sockets[i].swnd.size = 0;
        shm->sockets[i].rwnd.base = 0;
        shm->sockets[i].rwnd.size = 0;
        shm->sockets[i].rwnd.rwnd = WINDOW_SIZE;
        memset(shm->sockets[i].send_buffer, 0, sizeof(KTP_Message) * WINDOW_SIZE);
        memset(shm->sockets[i].recv_buffer, 0, sizeof(KTP_Message) * WINDOW_SIZE);
        memset(shm->sockets[i].swnd.messages, 0, sizeof(KTP_Message) * WINDOW_SIZE);
        memset(shm->sockets[i].swnd.timestamps, 0, sizeof(time_t) * WINDOW_SIZE);
        memset(shm->sockets[i].rwnd.messages, 0, sizeof(KTP_Message) * WINDOW_SIZE);
    }

    shm->drop_probability = P;  // Default from header
    std::cout << "[init_shared_memory] Shared memory initialized with ID: " << shmid << "\n";
    return shmid;
}

SharedMemory* attach_shared_memory(int shmid) {
    void* ptr = shmat(shmid, nullptr, 0);
    if (ptr == reinterpret_cast<void*>(-1)) {
        perror("[attach_shared_memory] shmat failed");
        return nullptr;
    }
    std::cout << "[attach_shared_memory] Attached at " << ptr << "\n";
    return static_cast<SharedMemory*>(ptr);
}

void detach_shared_memory(SharedMemory* shm_ptr) {
    if (shmdt(shm_ptr) == -1) {
        perror("[detach_shared_memory] shmdt failed");
    } else {
        std::cout << "[detach_shared_memory] Detached successfully\n";
    }
}

void cleanup_shared_memory(int shmid) {
    if (shmctl(shmid, IPC_RMID, nullptr) == -1) {
        perror("[cleanup_shared_memory] shmctl failed");
    } else {
        std::cout << "[cleanup_shared_memory] Shared memory " << shmid << " removed\n";
    }
}

int k_socket() {
    std::cout << "[k_socket] Creating KTP socket...\n";
    if (!shm) {
        std::cerr << "[k_socket] Shared memory not attached\n";
        return -1;
    }
    pid_t pid = getpid();
    for (int i = 0; i < MAX_SOCKETS; ++i) {
        if (shm->sockets[i].is_free) {
            shm->sockets[i].is_free = 0;
            shm->sockets[i].process_id = pid;
            shm->sockets[i].swnd.base = 0;
            shm->sockets[i].swnd.next_seq = 0;
            shm->sockets[i].swnd.size = 0;
            shm->sockets[i].rwnd.base = 0;
            shm->sockets[i].rwnd.size = 0;
            shm->sockets[i].rwnd.rwnd = WINDOW_SIZE;
            std::cout << "[k_socket] Allocated socket " << i << " to PID " << pid << "\n";
            return i;
        }
    }
    std::cout << "[k_socket] No space available\n";
    return ENOSPC; // updated from ENOSPACE to POSIX ENOSPC
}

int k_bind(int sockfd, const char* src_ip, int src_port,
           const char* dest_ip, int dest_port) {
    std::cout << "[k_bind] Binding socket " << sockfd << " to " << src_ip << ":" << src_port
              << ", dest " << dest_ip << ":" << dest_port << "...\n";
    if (!shm || sockfd < 0 || sockfd >= MAX_SOCKETS || shm->sockets[sockfd].is_free) {
        std::cerr << "[k_bind] Invalid socket\n";
        return ENOTCONN; // updated ENOTBOUND -> ENOTCONN
    }

    shm->sockets[sockfd].src_addr.sin_family = AF_INET;
    shm->sockets[sockfd].src_addr.sin_port = htons(src_port);
    inet_pton(AF_INET, src_ip, &shm->sockets[sockfd].src_addr.sin_addr);

    shm->sockets[sockfd].dest_addr.sin_family = AF_INET;
    shm->sockets[sockfd].dest_addr.sin_port = htons(dest_port);
    inet_pton(AF_INET, dest_ip, &shm->sockets[sockfd].dest_addr.sin_addr);

    return sockfd;
}

int k_sendto(int sockfd, const void* buf, size_t len, int /*flags*/) {
    std::cout << "[k_sendto] Sending from socket " << sockfd << "...\n";
    if (!shm || sockfd < 0 || sockfd >= MAX_SOCKETS || shm->sockets[sockfd].is_free) {
        std::cerr << "[k_sendto] Invalid socket\n";
        return ENOTCONN;
    }
    if (shm->sockets[sockfd].swnd.size >= WINDOW_SIZE) {
        std::cout << "[k_sendto] Sender window full (swnd.size="
                  << shm->sockets[sockfd].swnd.size
                  << ", base=" << shm->sockets[sockfd].swnd.base
                  << ", next_seq=" << shm->sockets[sockfd].swnd.next_seq << ")\n";
        return ENOSPC;
    }
    if (shm->sockets[sockfd].send_count >= WINDOW_SIZE) {
        std::cerr << "[k_sendto] Send buffer full\n";
        return ENOSPC;
    }

    KTP_Message msg{};
    msg.seq_num = shm->sockets[sockfd].swnd.next_seq % 256;
    msg.type = MSG_TYPE_DATA;
    size_t copy_len = (len < MESSAGE_SIZE) ? len : MESSAGE_SIZE;
    memcpy(msg.payload, buf, copy_len);
    msg.payload[copy_len] = '\0';

    shm->sockets[sockfd].send_buffer[shm->sockets[sockfd].send_count++] = msg;
    shm->sockets[sockfd].swnd.next_seq = (shm->sockets[sockfd].swnd.next_seq + 1) % 256;

    std::cout << "[k_sendto] Queued seq=" << (int)msg.seq_num << ": " << msg.payload << "\n";
    return static_cast<int>(copy_len);
}

int k_recvfrom(int sockfd, void* buf, size_t len, int /*flags*/) {
    std::cout << "[k_recvfrom] Receiving on socket " << sockfd << "...\n";
    if (!shm || sockfd < 0 || sockfd >= MAX_SOCKETS || shm->sockets[sockfd].is_free) {
        std::cerr << "[k_recvfrom] Invalid socket\n";
        return ENOTCONN;
    }
    if (shm->sockets[sockfd].recv_count > 0) {
        KTP_Message msg = shm->sockets[sockfd].recv_buffer[0];
        int msg_len = strnlen(msg.payload, MESSAGE_SIZE);
        if (msg_len > static_cast<int>(len)) msg_len = static_cast<int>(len);
        memcpy(buf, msg.payload, msg_len);
        for (int i = 1; i < shm->sockets[sockfd].recv_count; ++i) {
            shm->sockets[sockfd].recv_buffer[i - 1] = shm->sockets[sockfd].recv_buffer[i];
        }
        shm->sockets[sockfd].recv_count--;
        shm->sockets[sockfd].rwnd.rwnd = WINDOW_SIZE - shm->sockets[sockfd].recv_count;
        std::cout << "[k_recvfrom] Got seq=" << (int)msg.seq_num
                  << ": " << static_cast<char*>(buf)
                  << ", rwnd=" << shm->sockets[sockfd].rwnd.rwnd << "\n";
        return msg_len;
    }
    std::cout << "[k_recvfrom] No message available\n";
    return EAGAIN; // changed from ENOMESSAGE -> EAGAIN (standard for no data)
}

int k_close(int sockfd) {
    std::cout << "[k_close] Closing socket " << sockfd << "...\n";
    if (!shm || sockfd < 0 || sockfd >= MAX_SOCKETS || shm->sockets[sockfd].is_free) {
        std::cerr << "[k_close] Invalid socket\n";
        return ENOTCONN;
    }
    shm->sockets[sockfd].is_free = 1;
    shm->sockets[sockfd].process_id = 0;
    shm->sockets[sockfd].send_count = 0;
    shm->sockets[sockfd].recv_count = 0;
    shm->sockets[sockfd].swnd.size = 0;
    std::cout << "[k_close] Closed socket " << sockfd << "\n";
    return 0;
}
