#include "ClientThread.h"
#include "Messages.h"

#include <iostream>

ClientThreadClass::ClientThreadClass() {}

void ClientThreadClass::SendOrderRequest(std::string ip, int port, int id, int orders, int type) {
	customer_id = id;
	order_count = orders;
	request_type = type;

	if (!stub.Init(ip, port)) {
		std::cout << "Thread " << customer_id << " failed to connect" << std::endl;
		return;
	}

	for (int i = 0; i < order_count; i++) {
		CustomerRequest order;
		RobotInfo robot;

		order.SetOrder(customer_id, i, request_type);

		timer.Start();
		robot = stub.Order(order);
		timer.EndAndMerge();

		if (!robot.IsValid()) {
			break;	
		} 
	}
}

ClientTimer ClientThreadClass::GetTimer() {
	return timer;	
}

void ClientThreadClass::ScanOrders(std::string ip, int port, int max_customer_id, int type) {
	request_type = type;
	if (!stub.Init(ip, port)) {
		std::cout << "Thread failed to connect" << std::endl;
		return;
	}
	for (int id = 0; id < max_customer_id; id++) {
		CustomerRequest request;
		CustomerInfo record;
		request.SetOrder(id, -1, request_type);

		timer.Start();
		record = stub.ReadRecord(request);
		timer.EndAndMerge();

		if (record.GetCustomerId() == -1) {
			std::cout << "Server unable to send response for customer #" << id << std::endl;
			break;
		} 
		if (record.GetLastOrderNumber() == -1) {
			continue;
		}
		std::cout << record.GetCustomerId() << "\t" << record.GetLastOrderNumber() << std::endl;
	}
}

void ClientThreadClass::ReadRecordRequest(std::string ip, int port, int id, int orders, int type) {
	customer_id = id;
	request_type = type;
	if (!stub.Init(ip, port)) {
		std::cout << "Thread " << customer_id << " failed to connect" << std::endl;
		return;
	}
	for (int i = 0; i < orders; i++) {
		CustomerRequest request;
		CustomerInfo record;
		request.SetOrder(customer_id, -1, request_type);

		timer.Start();
		record = stub.ReadRecord(request);
		timer.EndAndMerge();

		if (record.GetCustomerId() == -1) {
			std::cout << "Server didn't respond for customer " << customer_id << std::endl;
			break;
		} 
	}
}
