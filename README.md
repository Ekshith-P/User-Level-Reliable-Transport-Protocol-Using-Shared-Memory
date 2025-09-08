# KTP: Reliable Data Transfer Protocol in C++

This project is a **C++ implementation of a custom reliable transport protocol (KTP)** built on top of UDP.  
It uses a **daemon-based architecture with shared memory IPC**, effectively separating the protocol logic from the user-facing applications.

The system simulates an unreliable network (with configurable packet drop probability) and ensures **reliable, in-order data delivery** using a **sliding window mechanism**.

---

## ✨ Features

- **Reliable Data Transfer:** Acknowledgments (ACKs) + retransmissions for guaranteed delivery over UDP.  
- **Sliding Window Protocol:** Go-Back-N style mechanism for flow control and error recovery.  
- **Daemon Architecture:**  
  - Central daemon (`initksocket`) manages protocol logic, packet handling, and socket states.  
  - Multi-threaded design:  
    - **Thread R (Receiver):** Handles incoming packets & ACKs.  
    - **Thread S (Sender):** Transmits/retransmits packets based on timeout.  
    - **Thread G (Garbage Collector):** Cleans up unused socket resources.  
- **Shared Memory IPC:** Fast communication between user applications (`user1`, `user2`) and the daemon.  
- **Socket Garbage Collection:** Automatically frees socket resources if a user process terminates unexpectedly.  

---

## 🏗 Architecture

The project has 3 main components:

1. **Daemon (`initksocket`)**  
   - Core system process, manages shared memory & protocol logic.  
   - Runs three threads (Receiver, Sender, Garbage Collector).  

2. **KTP Library (`libksocket.a`)**  
   - Provides API:  
     - `k_socket`  
     - `k_bind`  
     - `k_sendto`  
     - `k_recvfrom`  
     - `k_close`  
   - User apps interact with this API, which communicates with the daemon via shared memory.  

3. **User Applications (`user1`, `user2`)**  
   - `user1`: Sender, reads `input.txt` and sends data via `k_sendto`.  
   - `user2`: Receiver, writes received data to `output.txt` using `k_recvfrom`.  

---

## ⚡ How to Compile & Run

### 1. Prerequisites
Ensure you have `g++` and `make` installed:  

sudo apt-get update
sudo apt-get install build-essential
2.  **Compile
Clone the repo and run:

make
3.  **Run the System
Open 3 terminals:

Terminal 1 – Start the Daemon
./initksocket
Example output:

[initksocket] Shared memory created with ID: 123456
[initksocket] Threads R, S, and G started
[initksocket] Run user applications with shmid=123456
Terminal 2 – Start the Receiver

./user2 <shmid> output.txt
Terminal 3 – Start the Sender

./user1 <shmid> input.txt [<drop_probability>]
Example:

./user1 123456 input.txt 0.2
4.  **Verify File Transfer

diff input.txt output.txt
No output → files are identical ✅

5.  **Clean Up
Stop daemon (Ctrl + C in Terminal 1).

👨‍💻 Author
Name: Ekshith Pakala
Roll No: 22MT10038
