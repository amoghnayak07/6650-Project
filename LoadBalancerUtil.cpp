#include "LoadBalancerUtil.h"
#include "Messages.h"
#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstring>
#include <thread>
#include <sys/select.h>

LoadBalancerConnection::LoadBalancerConnection(int sock_fd, BackendFactory* factory)
    : socket_fd(sock_fd), backend(factory), buffer_size(4096) {}

LoadBalancerConnection::~LoadBalancerConnection() {
    if (socket_fd >= 0) {
        close(socket_fd);
    }
    if (backend_socket_fd >= 0) {
        close(backend_socket_fd);
    }
}

bool LoadBalancerConnection::ConnectToBackend() {
    if (!backend) return false;
    backend_socket_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (backend_socket_fd < 0) {
        std::cerr << "Failed to create backend socket" << std::endl;
        return false;
    }

    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(backend->port);
    memset(&server_addr.sin_zero, 0, sizeof(server_addr.sin_zero));

    if (inet_pton(AF_INET, backend->ip.c_str(), &server_addr.sin_addr) <= 0) {
        std::cerr << "Invalid backend IP: " << backend->ip << std::endl;
        close(backend_socket_fd);
        backend_socket_fd = -1;
        return false;
    }

    if (connect(backend_socket_fd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        std::cerr << "Failed to connect to backend " << backend->id << " at "
                  << backend->ip << ":" << backend->port << std::endl;
        close(backend_socket_fd);
        backend_socket_fd = -1;
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
    if (socket_fd < 0 || backend_socket_fd < 0) return false;

    char buffer[4096];
    fd_set readfds;
    int max_fd = (socket_fd > backend_socket_fd) ? socket_fd : backend_socket_fd;

    bool client_open = true;
    bool backend_open = true;

    while (client_open && backend_open) {
        FD_ZERO(&readfds);
        if (client_open) FD_SET(socket_fd, &readfds);
        if (backend_open) FD_SET(backend_socket_fd, &readfds);

        int activity = select(max_fd + 1, &readfds, NULL, NULL, NULL);
        if (activity < 0) {
            perror("select");
            break;
        }

        if (client_open && FD_ISSET(socket_fd, &readfds)) {
            int n = recv(socket_fd, buffer, sizeof(buffer), 0);
            if (n <= 0) {
                client_open = false;
            } else {
                if (send(backend_socket_fd, buffer, n, 0) <= 0) {
                    backend_open = false;
                }
            }
        }

        if (backend_open && FD_ISSET(backend_socket_fd, &readfds)) {
            int n = recv(backend_socket_fd, buffer, sizeof(buffer), 0);
            if (n <= 0) {
                backend_open = false;
            } else {
                if (send(socket_fd, buffer, n, 0) <= 0) {
                    client_open = false;
                }
            }
        }
    }

    if (socket_fd >= 0) { close(socket_fd); socket_fd = -1; }
    if (backend_socket_fd >= 0) { close(backend_socket_fd); backend_socket_fd = -1; }

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

BackendFactory* RoundRobinRouter::GetRandomFactory() {
    std::lock_guard<std::mutex> lg(router_mutex);
    if (factories.empty()) {
        return nullptr;
    }
    size_t idx = static_cast<size_t>(rand()) % factories.size();
    return &factories[idx];
}

BackendFactory* RoundRobinRouter::GetPrimaryFactory() {
    std::lock_guard<std::mutex> lg(router_mutex);
    if (factories.empty()) {
        return nullptr;
    }
    
    // Query each factory to find which one is primary
    for (auto& factory : factories) {
        int query_socket = socket(AF_INET, SOCK_STREAM, 0);
        if (query_socket < 0) {
            continue;
        }

        struct sockaddr_in server_addr;
        server_addr.sin_family = AF_INET;
        server_addr.sin_port = htons(factory.port);
        memset(&server_addr.sin_zero, 0, sizeof(server_addr.sin_zero));

        if (inet_pton(AF_INET, factory.ip.c_str(), &server_addr.sin_addr) <= 0) {
            close(query_socket);
            continue;
        }

        if (connect(query_socket, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
            close(query_socket);
            continue;
        }

        // Send STATE_QUERY role identifier
        RoleIdentifier idty;
        idty.SetRole(RoleIdentifier::STATE_QUERY);
        char buffer[4];
        idty.Marshal(buffer);
        if (send(query_socket, buffer, idty.Size(), 0) <= 0) {
            close(query_socket);
            continue;
        }

        // Send state query
        ServerStateQuery query;
        char query_buffer[4];
        query.Marshal(query_buffer);
        if (send(query_socket, query_buffer, query.Size(), 0) <= 0) {
            close(query_socket);
            continue;
        }

        // Receive state response
        char response_buffer[64];
        int n = recv(query_socket, response_buffer, sizeof(response_buffer), 0);
        close(query_socket);

        if (n > 0) {
            ServerStateResponse response;
            response.Unmarshal(response_buffer);
            
            if (response.IsPrimary()) {
                std::cout << "Discovered primary: factory " << response.GetFactoryId() << std::endl;
                return &factory;
            }
        }
    }
    
    // Fallback: return first factory
    std::cout << "Could not discover primary, using first factory" << std::endl;
    return &factories[0];
}
