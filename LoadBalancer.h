#ifndef LOADBALANCER_H
#define LOADBALANCER_H

#include <vector>
#include <string>
#include <mutex>
#include "ClientSocket.h"
#include "ServerSocket.h"

enum BalancingStrategy {
    ROUND_ROBIN,
    RANDOM
    // Add LEAST_CONNECTIONS etc. here
};

struct BackendNode {
    std::string ip;
    int port;
};

class LoadBalancer {
private:
    std::vector<BackendNode> backends;
    int primary_index;
    
    // State for Round Robin
    int current_rr_index;
    std::mutex strategy_mutex;

    BalancingStrategy read_strategy;

public:
    LoadBalancer(std::vector<BackendNode> nodes, int primary_idx, BalancingStrategy strategy);

    // Get the destination for a Write request (Always Primary)
    BackendNode GetWriteTarget();

    // Get the destination for a Read request (Based on Strategy)
    BackendNode GetReadTarget();

    // The core proxy function
    static void ProxyConnection(std::unique_ptr<ServerSocket> client_socket, BackendNode target);
    
    // Helper to pipe data in one direction
    static void RelayTraffic(Socket* source, Socket* dest);
};

#endif