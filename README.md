# KTP: Reliable Data Transfer Protocol in C++

This project is a C++ implementation of a custom reliable transport protocol (KTP) built on top of UDP. It uses a daemon-based architecture with shared memory for inter-process communication, effectively separating the protocol's complex logic from the user-facing applications.

The system simulates an unreliable network by allowing for configurable packet drop probability, and it ensures reliable, in-order data delivery using a sliding window mechanism.

***

## Key Features 🚀

* **Reliable Data Transfer:** Implements acknowledgments (ACKs) and retransmissions to ensure data delivery over the unreliable UDP protocol.
* **Sliding Window Protocol:** Uses a Go-Back-N-like sliding window for efficient flow control and error recovery.
* **Daemon Architecture:** A central daemon (`initksocket`) manages all protocol logic, including packet handling and state management for all connections.
* **Shared Memory IPC:** User applications (`user1`, `user2`) communicate with the daemon through a fast and efficient shared memory segment.
* **Socket Garbage Collection:** A dedicated thread in the daemon cleans up resources allocated for sockets if the parent user application terminates unexpectedly.

***

## Architecture Overview

The project consists of three main components that work together:

1.  **The Daemon (`initksocket`)**
    This is the core of the system. When launched, it creates the shared memory segment and starts three concurrent threads:
    * **Thread R (Receiver):** Listens on the UDP ports for all incoming packets. It handles both data packets (placing them in the appropriate receive buffer) and ACK packets (updating the sender's window).
    * **Thread S (Sender):** Periodically scans the sending windows of all active sockets. It transmits new data packets and re-transmits any packets that have timed out without receiving an ACK.
    * **Thread G (Garbage Collector):** Periodically checks if the user processes that created sockets are still running. If a process is gone, it frees the corresponding socket resources to prevent memory leaks.

2.  **The KTP Library (`libksocket.a`)**
    This static library provides a simple API (`k_socket`, `k_bind`, `k_sendto`, `k_recvfrom`, `k_close`) for user applications. When an application calls one of these functions, it's actually just reading from or writing to the shared memory segment. The daemon's threads handle the rest.

3.  **User Applications (`user1`, `user2`)**
    These are the client programs that use the KTP library to send and receive data.
    * `user1` acts as the sender, reading a file and pushing its contents into the send buffer via `k_sendto`.
    * `user2` acts as the receiver, pulling data from the receive buffer via `k_recvfrom` and writing it to a new file.

***

## How to Compile and Run

Follow these steps to get the project running. You will need three separate terminal windows.

### 1. Prerequisites

Ensure you have `g++` and `make` installed on your Linux system.

sudo apt-get update
sudo apt-get install build-essential

2. Compile the Project
Clone the repository and run the make command in the project's root directory. This will compile all the C++ source files and create the executables.
make
3. Run the System
Terminal 1: Start the Daemon
First, run the initksocket executable. It will create the shared memory and print its unique ID (shmid). Keep this terminal open.

./initksocket
Example Output:

[initksocket] Shared memory created with ID: 123456
[initksocket] Threads R, S, and G started
[initksocket] Run user applications with shmid=123456
[initksocket] Press Ctrl+C to exit
Terminal 2: Start the Receiver
In a new terminal, run the user2 executable. You must provide the shmid from the first terminal and a name for the output file.


# Usage: ./user2 <shmid> <output_filename>
./user2 123456 output.txt
Terminal 3: Start the Sender
In a third terminal, run the user1 executable. Provide the same shmid, the input file (input.txt), and an optional packet drop probability (e.g., 0.2 for 20%).


# Usage: ./user1 <shmid> <input_filename> [<drop_probability>]
./user1 123456 input.txt 0.2
The sender will now transfer the file to the receiver. You will see log messages in all three terminals.

4. Verify and Clean Up
Verify the Transfer: Once the sender is finished, you can check if the file was transferred correctly.


diff input.txt output.txt
If there is no output, the files are identical. ✅

Shut Down: Stop the daemon by going to Terminal 1 and pressing Ctrl+C.

Clean Build Files: To remove all compiled object files and executables, run:
make clean

Author
Name: Ekshith Pakala
Roll: 22MT10038

