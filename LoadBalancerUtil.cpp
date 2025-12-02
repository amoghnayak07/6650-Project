#include "LoadBalancerUtil.h"
#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstring>
#include <thread>
#include <select.h>

LoadBalancerConnection::LoadBalancerConnection(int sock_fd, BackendFactory* factory)
    : socket_fd(sock_fd), backend(factory), buffer_size(4096) {}

LoadBalancerConnection::~LoadBalancerConnection() {
    if (socket_fd >= 0) {
        close(socket_fd);
    }
}

bool LoadBalancerConnection::ConnectToBackend() {
    if (!backend) return false;
    
    int backend_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (backend_socket < 0) {
        std::cerr << "Failed to create backend socket" << std::endl;
        return false;
    }
    
    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(backend->port);
    
    if (inet_pton(AF_INET, backend->ip.c_str(), &server_addr.sin_addr) <= 0) {
        std::cerr << "Invalid backend IP: " << backend->ip << std::endl;
        close(backend_socket);
        return false;
    }
    
    if (connect(backend_socket, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        std::cerr << "Failed to connect to backend " << backend->id << " at " 
                  << backend->ip << ":" << backend->port << std::endl;
        close(backend_socket);
        return false;
    }
    
    std::cout << "Connected to backend factory " << backend->id 
              << " at " << backend->ip << ":" << backend->port << std::endl;
    
    return true;
}

bool LoadBalancerConnection::ProxyConnection() {
    if (!ConnectToBackend()) {
        return false;
    }
    
    return Forward();
}

bool LoadBalancerConnection::Forward() {
    // This will be implemented in LoadBalancerMain to handle bidirectional proxying
    return true;
}

void RoundRobinRouter::AddFactory(const BackendFactory& factory) {
    std::lock_guard<std::mutex> lg(router_mutex);
    factories.push_back(factory);
    std::cout << "Added factory " << factory.id << " to load balancer" << std::endl;
}

BackendFactory* RoundRobinRouter::GetNextFactory() {
    std::lock_guard<std::mutex> lg(router_mutex);
    if (factories.empty()) {
        return nullptr;
    }
    
    BackendFactory* factory = &factories[current_index];
    current_index = (current_index + 1) % factories.size();
    return factory;
}

BackendFactory* RoundRobinRouter::GetPrimaryFactory() {
    std::lock_guard<std::mutex> lg(router_mutex);
    if (factories.empty()) {
        return nullptr;
    }
    
    // Primary is assumed to be the first factory (id 0)
    for (auto& factory : factories) {
        if (factory.id == 0) {
            return &factory;
        }
    }
    
    // If no factory with id 0, return the first one
    return &factories[0];
}
