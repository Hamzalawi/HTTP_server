# Multithreaded HTTP Server in C

A lightweight, high-performance HTTP server written in C using native Linux system calls. This project demonstrates core systems programming concepts, including low-level network socket programming, multi-threading (`pthread`), and a thread-safe circular task queue using condition variables to prevent busy-waiting.

## 🚀 Features

* **Socket Programming:** Manages low-level TCP/IP connections using the `sys/socket.h` API.
* **Thread Pool Architecture:** Avoids the overhead of creating a new thread for every incoming connection by managing a fixed pool of persistent worker threads.
* **Thread-Safe Circular Queue:** Implements a custom ring-buffer style task queue utilizing POSIX mutexes (`pthread_mutex_t`) and condition variables (`pthread_cond_t`).
* **Efficient Synchronization:** Eliminates CPU polling/busy-waiting by putting worker threads to sleep until a new request is successfully enqueued.

---

## 🛠️ System Architecture

The server operates using a classic **Producer-Consumer** pattern:

1.  **The Producer (Main Thread):** Listens on port `8080` for incoming TCP connections. When a client connects, the main thread accepts the connection socket and pushes (`enqueue`) its file descriptor onto the circular queue.
2.  **The Queue:** A synchronized FIFO ring buffer that safely coordinates data transfer between threads.
3.  **The Consumers (Worker Threads):** A fixed pool of 4 threads waiting on a condition variable. When a socket is enqueued, one worker wakes up, pops (`dequeue`) the socket, processes the HTTP request, returns a basic HTML response, and safely closes the connection.

---

## 💻 Getting Started

### Prerequisites

This server relies on standard Linux POSIX libraries (`pthread`, `sys/socket`). It is best run on Linux, macOS, or WSL (Windows Subsystem for Linux).

### Compilation

Compile the program using `gcc` or `clang`. Don't forget to link the `pthread` library using the `-pthread` flag:

```bash
gcc -pthread server.c -o server