#ifndef __LOAD_BALANCER_UTIL_H__
#define __LOAD_BALANCER_UTIL_H__

#include <string>
#include <vector>
#include <memory>
#include <thread>
#include <mutex>

class BackendFactory {
public:
    int id;
    std::string ip;
    int port;
    
    BackendFactory(int factory_id, const std::string& factory_ip, int factory_port)
        : id(factory_id), ip(factory_ip), port(factory_port) {}
};

class LoadBalancerConnection {
private:
    int socket_fd;
    BackendFactory* backend;
    int buffer_size;
    
public:
    LoadBalancerConnection(int sock_fd, BackendFactory* factory);
    ~LoadBalancerConnection();
    
    bool ConnectToBackend();
    bool ProxyConnection();
    bool Forward();
    BackendFactory* GetBackend() const { return backend; }
    int GetSocketFd() const { return socket_fd; }
};

class RoundRobinRouter {
private:
    std::vector<BackendFactory> factories;
    size_t current_index;
    mutable std::mutex router_mutex;
    
public:
    RoundRobinRouter() : current_index(0) {}
    
    void AddFactory(const BackendFactory& factory);
    BackendFactory* GetNextFactory();
    BackendFactory* GetPrimaryFactory();
    size_t GetFactoryCount() const { return factories.size(); }
};

#endif // __LOAD_BALANCER_UTIL_H__
