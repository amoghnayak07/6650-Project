#include <array>
#include <iostream> 
#include <iomanip> 
#include <thread> 
#include <vector> 
#include <memory>

#include "ClientSocket.h"
#include "ClientThread.h"
#include "ClientTimer.h"

int main(int argc, char *argv[]) {
	std::string ip;
	int port;
	int num_customers;
	int num_orders;
	int request_type;
	ClientTimer timer;

	std::vector<std::shared_ptr<ClientThreadClass>> client_vector;
	std::vector<std::thread> thread_vector;
	
	if (argc < 6) {
		std::cout << "not enough arguments" << std::endl;
		std::cout << argv[0] << " [ip] [port #] [# customers] ";
		std::cout << "[# orders] [order type 1, 2 or 3]" << std::endl;
		return 0;
	}

	ip = argv[1];
	port = atoi(argv[2]);
	num_customers = atoi(argv[3]);
	num_orders = atoi(argv[4]);
	request_type = atoi(argv[5]);

	timer.Start();
	if (request_type == 2){
		for (int i = 0; i < num_customers; i++) {
			auto client_worker = std::shared_ptr<ClientThreadClass>(new ClientThreadClass());
			std::thread client_thread(&ClientThreadClass::ReadRecordRequest, client_worker,
					ip, port, i, num_orders, request_type);
			client_vector.push_back(std::move(client_worker));
			thread_vector.push_back(std::move(client_thread));
		}
	} else if (request_type == 3){
		auto client_worker = std::shared_ptr<ClientThreadClass>(new ClientThreadClass());
		std::thread client_thread(&ClientThreadClass::ScanOrders, client_worker,
				ip, port, num_orders, 2);
		client_vector.push_back(std::move(client_worker));
		thread_vector.push_back(std::move(client_thread));
	} else {
		for (int i = 0; i < num_customers; i++) {
			auto client_worker = std::shared_ptr<ClientThreadClass>(new ClientThreadClass());
			std::thread client_thread(&ClientThreadClass::SendOrderRequest, client_worker,
					ip, port, i, num_orders, request_type);

			client_vector.push_back(std::move(client_worker));
			thread_vector.push_back(std::move(client_thread));
		}
	}
	for (auto& th : thread_vector) {
		th.join();
	}
	timer.End();

	for (auto& clientWorkerPtr : client_vector) {
		timer.Merge(clientWorkerPtr->GetTimer());	
	}
	timer.PrintStats();
	return 1;
}
