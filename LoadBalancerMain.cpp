#include <iostream>
#include <vector>
#include <thread>
#include <mutex>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstring>
#include <cstdlib>
#include <sys/select.h>

#include "LoadBalancerUtil.h"

class LoadBalancer {
private:
    int write_port;
    int read_port;
    int write_listen_socket;
    int read_listen_socket;
    RoundRobinRouter read_router;
    BackendFactory* primary_factory;
    bool running;
    std::string read_policy; // "roundrobin" or "random"
    
public:
        LoadBalancer(int w_port, int r_port) 
        : write_port(w_port), read_port(r_port), 
          write_listen_socket(-1), read_listen_socket(-1),
                    primary_factory(nullptr), running(false) {
                const char* env = getenv("LB_READ_POLICY");
                if (env && *env) read_policy = std::string(env);
                if (read_policy.empty()) read_policy = "roundrobin";
        }
    
    bool Initialize() {
        write_listen_socket = socket(AF_INET, SOCK_STREAM, 0);
        if (write_listen_socket < 0) {
            std::cerr << "Failed to create write listen socket" << std::endl;
            return false;
        }
        
        read_listen_socket = socket(AF_INET, SOCK_STREAM, 0);
        if (read_listen_socket < 0) {
            std::cerr << "Failed to create read listen socket" << std::endl;
            close(write_listen_socket);
            return false;
        }
        
        // Set socket options to allow reuse
        int opt = 1;
        setsockopt(write_listen_socket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
        setsockopt(read_listen_socket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
        
        // Bind write port
        struct sockaddr_in write_addr;
        write_addr.sin_family = AF_INET;
        write_addr.sin_addr.s_addr = INADDR_ANY;
        write_addr.sin_port = htons(write_port);
        
        if (bind(write_listen_socket, (struct sockaddr*)&write_addr, sizeof(write_addr)) < 0) {
            std::cerr << "Failed to bind write port " << write_port << std::endl;
            close(write_listen_socket);
            close(read_listen_socket);
            return false;
        }
        
        // Bind read port
        struct sockaddr_in read_addr;
        read_addr.sin_family = AF_INET;
        read_addr.sin_addr.s_addr = INADDR_ANY;
        read_addr.sin_port = htons(read_port);
        
        if (bind(read_listen_socket, (struct sockaddr*)&read_addr, sizeof(read_addr)) < 0) {
            std::cerr << "Failed to bind read port " << read_port << std::endl;
            close(write_listen_socket);
            close(read_listen_socket);
            return false;
        }
        
        listen(write_listen_socket, 5);
        listen(read_listen_socket, 5);
        
        std::cout << "Load Balancer initialized:" << std::endl;
        std::cout << "  Write port: " << write_port << " (primary-only)" << std::endl;
        std::cout << "  Read port: " << read_port << " (policy: " << read_policy << ")" << std::endl;
        
        return true;
    }
    
    void AddBackendFactory(int factory_id, const std::string& ip, int port) {
        BackendFactory factory(factory_id, ip, port);
        read_router.AddFactory(factory);
        primary_factory = read_router.GetPrimaryFactory();
    }
    
    void HandleWriteConnection(int client_socket) {
        // Refresh primary on each write to handle failover/promotions
        primary_factory = read_router.GetPrimaryFactory();
        if (!primary_factory) {
            std::cerr << "No primary factory available" << std::endl;
            close(client_socket);
            return;
        }
        LoadBalancerConnection conn(client_socket, primary_factory);
        if (!conn.ProxyConnection()) {
            std::cerr << "Failed to proxy write to primary factory " << primary_factory->id << std::endl;
            return;
        }
        std::cout << "Write request routed to primary factory " << primary_factory->id << std::endl;
    }
    
    void HandleReadConnection(int client_socket) {
        BackendFactory* factory = nullptr;
        if (read_policy == "random") {
            factory = read_router.GetRandomFactory();
        } else {
            factory = read_router.GetNextFactory();
        }
        if (!factory) {
            std::cerr << "No backend factories available" << std::endl;
            close(client_socket);
            return;
        }
        LoadBalancerConnection conn(client_socket, factory);
        if (!conn.ProxyConnection()) {
            std::cerr << "Failed to proxy read to factory " << factory->id << std::endl;
            return;
        }
        std::cout << "Read request routed to factory " << factory->id << " (policy: " << read_policy << ")" << std::endl;
    }
    
    void WriteServerThread() {
        running = true;
        while (running) {
            struct sockaddr_in client_addr;
            socklen_t client_addr_len = sizeof(client_addr);
            
            int client_socket = accept(write_listen_socket, 
                                      (struct sockaddr*)&client_addr, 
                                      &client_addr_len);
            if (client_socket < 0) {
                continue;
            }
            
            std::thread handler(&LoadBalancer::HandleWriteConnection, this, client_socket);
            handler.detach();
        }
    }
    
    void ReadServerThread() {
        running = true;
        while (running) {
            struct sockaddr_in client_addr;
            socklen_t client_addr_len = sizeof(client_addr);
            
            int client_socket = accept(read_listen_socket, 
                                      (struct sockaddr*)&client_addr, 
                                      &client_addr_len);
            if (client_socket < 0) {
                continue;
            }
            
            std::thread handler(&LoadBalancer::HandleReadConnection, this, client_socket);
            handler.detach();
        }
    }
    
    void Start() {
        std::thread write_thread(&LoadBalancer::WriteServerThread, this);
        std::thread read_thread(&LoadBalancer::ReadServerThread, this);
        
        write_thread.join();
        read_thread.join();
    }
    
    ~LoadBalancer() {
        running = false;
        if (write_listen_socket >= 0) close(write_listen_socket);
        if (read_listen_socket >= 0) close(read_listen_socket);
    }
};

int main(int argc, char* argv[]) {
    
    if (argc < 4 || argc < 4 + (atoi(argv[3]) * 3)) {
        std::cout << "Usage: " << argv[0] << " [write_port] [read_port] [# factories]" << std::endl;
        std::cout << "        (repeat [factory_id] [IP] [port])" << std::endl;
        return 1;
    }
    
    int write_port = atoi(argv[1]);
    int read_port = atoi(argv[2]);
    int num_factories = atoi(argv[3]);
    
    LoadBalancer lb(write_port, read_port);
    
    if (!lb.Initialize()) {
        std::cerr << "Failed to initialize load balancer" << std::endl;
        return 1;
    }
    
    // Parse factory information
    for (int i = 0; i < num_factories; i++) {
        int factory_id = atoi(argv[4 + i * 3]);
        std::string ip = argv[5 + i * 3];
        int port = atoi(argv[6 + i * 3]);
        lb.AddBackendFactory(factory_id, ip, port);
    }
    
    std::cout << "Load Balancer starting..." << std::endl;
    lb.Start();
    
    return 0;
}
