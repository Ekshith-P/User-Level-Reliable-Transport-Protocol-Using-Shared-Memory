# User-Level-Reliable-Transport-Protocol-Using-Shared-Memory

## Overview
This project implements a **user-level reliable transport protocol (KTP)** in C using **shared memory**.  
It provides reliable communication between two processes, simulating packet loss, acknowledgments, and retransmissions similar to TCP—but entirely in user space.

## Features
- Reliable message delivery with **sequence numbers** and **ACKs**
- **Sliding window** for flow control
- **Simulated packet loss** for testing
- Automatic retransmission for lost packets
- **Garbage collector thread** to clean up terminated processes

## Files
- `ksocket.h` / `ksocket.c` – KTP API and internal threads
- `initksocket.c` – Initializes shared memory and threads
- `user1.c` – Example sender program
- `user2.c` – Example receiver program
- `Makefile`s – Build scripts for library and executables

## Requirements
- GCC compiler
- POSIX OS (Linux/macOS)
- `pthread` library
- Shared memory support

## Build & Run

### Build
make -C ksocket_lib   # Build libksocket.a
make -C user_programs # Build user1 and user2
make -C initksocket   # Build initksocket

Author
Ekshith Pakala
GitHub: Ekshith-P
