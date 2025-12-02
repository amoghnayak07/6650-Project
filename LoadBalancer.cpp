#include "LoadBalancer.h"
#include <iostream>
#include <thread>
#include <cstdlib>
#include <ctime>

LoadBalancer::LoadBalancer(std::vector<BackendNode> nodes, int primary_idx, BalancingStrategy strategy)
    : backends(nodes), primary_index(primary_idx), current_rr_index(0), read_strategy(strategy) {
    std::srand(std::time(nullptr));
}

BackendNode LoadBalancer::GetWriteTarget() {
    // Write requests always go to the designated primary
    if (primary_index >= 0 && primary_index < backends.size()) {
        return backends[primary_index];
    }
    return backends[0]; // Fallback
}

BackendNode LoadBalancer::GetReadTarget() {
    std::lock_guard<std::mutex> lock(strategy_mutex);
    
    if (backends.empty()) return {"", 0};

    int index = 0;
    switch (read_strategy) {
        case RANDOM:
            index = std::rand() % backends.size();
            break;
        case ROUND_ROBIN:
        default:
            index = current_rr_index;
            current_rr_index = (current_rr_index + 1) % backends.size();
            break;
    }
    return backends[index];
}

void LoadBalancer::RelayTraffic(Socket* source, Socket* dest) {
    const int BUFFER_SIZE = 4096;
    char buffer[BUFFER_SIZE];
    
    // Keep reading and writing until connection closes or error
    while (true) {
        int bytes_read = source->Recv(buffer, BUFFER_SIZE, 0);
        if (bytes_read <= 0) break; // Closed or Error
        
        int bytes_sent = dest->Send(buffer, bytes_read, 0);
        if (bytes_sent <= 0) break; // Closed or Error
    }
    
    // If one side closes, close the other to signal termination
    // (The Socket destructor or Close() method handles shutdown)
    // We can explicitly close to unblock the other thread if implemented with select,
    // but with blocking sockets in threads, this loop just exits.
}

void LoadBalancer::ProxyConnection(std::unique_ptr<ServerSocket> client_socket, BackendNode target) {
    ClientSocket backend_socket;
    
    if (!backend_socket.Init(target.ip, target.port)) {
        std::cerr << "LB: Failed to connect to backend " << target.ip << ":" << target.port << std::endl;
        return;
    }

    // We need raw pointers for the threads
    Socket* client_ptr = client_socket.get();
    Socket* backend_ptr = &backend_socket;

    // Spawn threads for bidirectional piping
    // Thread 1: Client -> Backend
    std::thread upstream([client_ptr, backend_ptr]() {
        LoadBalancer::RelayTraffic(client_ptr, backend_ptr);
        backend_ptr->Close(); // Close backend to signal downstream thread
    });

    // Thread 2: Backend -> Client
    std::thread downstream([client_ptr, backend_ptr]() {
        LoadBalancer::RelayTraffic(backend_ptr, client_ptr);
        client_ptr->Close(); // Close client to signal upstream thread
    });

    upstream.join();
    downstream.join();
    
    // socket destructors will clean up
}