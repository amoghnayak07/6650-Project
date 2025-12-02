#include <iostream>
#include <thread>
#include <vector>
#include <string>
#include "LoadBalancer.h"

void RequestListener(int port, LoadBalancer* lb, bool is_write_port) {
    ServerSocket server_socket;
    if (!server_socket.Init(port)) {
        std::cerr << "LB: Failed to bind to port " << port << std::endl;
        return;
    }
    
    std::cout << "LB listening on " << (is_write_port ? "Write" : "Read") 
              << " port " << port << std::endl;

    while (true) {
        std::unique_ptr<ServerSocket> client = server_socket.Accept();
        if (client) {
            BackendNode target;
            if (is_write_port) {
                target = lb->GetWriteTarget();
            } else {
                target = lb->GetReadTarget();
            }

            // Handle proxying in a detached thread
            std::thread proxy_thread(&LoadBalancer::ProxyConnection, 
                                   std::move(client), target);
            proxy_thread.detach();
        }
    }
}

int main(int argc, char* argv[]) {
    // Usage: ./load_balancer [write_port] [read_port] [primary_idx] [N_factories] [ip1] [port1] ...
    if (argc < 5) {
        std::cout << "Usage: " << argv[0] << " [write_port] [read_port] [primary_idx] [count] ([ip] [port])..." << std::endl;
        return 1;
    }

    int write_port = atoi(argv[1]);
    int read_port = atoi(argv[2]);
    int primary_idx = atoi(argv[3]);
    int count = atoi(argv[4]);

    std::vector<BackendNode> nodes;
    for (int i = 0; i < count; i++) {
        std::string ip = argv[5 + (i * 2)];
        int port = atoi(argv[6 + (i * 2)]);
        nodes.push_back({ip, port});
    }

    // Policy can be changed here or via args
    LoadBalancer lb(nodes, primary_idx, ROUND_ROBIN);

    std::thread write_listener(RequestListener, write_port, &lb, true);
    std::thread read_listener(RequestListener, read_port, &lb, false);

    write_listener.join();
    read_listener.join();

    return 0;
}