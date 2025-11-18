#include "ClientStub.h"
#include <iostream>
ClientStub::ClientStub() {}

int ClientStub::Init(std::string ip, int port) {
	if (socket.Init(ip, port) == 0) {
		return 0;
	}
	RoleIdentifier id;
	id.SetRole(RoleIdentifier::CLIENT);
	char buffer[id.Size()];
	id.Marshal(buffer);
	int size = id.Size();
	return socket.Send(buffer, size, 0);
}

RobotInfo ClientStub::Order(CustomerRequest order) {
	RobotInfo info;
	char buffer[32];
	int size;
	order.Marshal(buffer);
	size = order.Size();
	
	if(socket.Send(buffer, size, 0) <=0) {
		return info;
	}
	size = info.Size();
	if (socket.Recv(buffer, size, 0) <=0) {
		return info;
	}
	info.Unmarshal(buffer);
	return info;
}

CustomerInfo ClientStub::ReadRecord(CustomerRequest request) {
	CustomerInfo record;
	char buffer[32];
	int size;
	request.Marshal(buffer);
	size = request.Size();

	if (!socket.Send(buffer, size, 0)) {
		return record;
	}
	size = record.Size();
	if (!socket.Recv(buffer, size, 0)) {
		return record;
	}
	record.Unmarshal(buffer);
	return record;
}