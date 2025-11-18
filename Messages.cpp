#include <cstring>
#include <iostream>

#include <arpa/inet.h>
#include "Messages.h"

CustomerRequest::CustomerRequest()
{
	customer_id = -1;
	order_number = -1;
	request_type = -1;
}

void CustomerRequest::SetOrder(int id, int number, int type)
{
	customer_id = id;
	order_number = number;
	request_type = type;
}

int CustomerRequest::GetCustomerId() { return customer_id; }
int CustomerRequest::GetOrderNumber() { return order_number; }
int CustomerRequest::GetRequestType() { return request_type; }

int CustomerRequest::Size()
{
	return sizeof(customer_id) + sizeof(order_number) + sizeof(request_type);
}

void CustomerRequest::Marshal(char *buffer)
{
	int net_customer_id = htonl(customer_id);
	int net_order_number = htonl(order_number);
	int net_request_type = htonl(request_type);
	int offset = 0;
	memcpy(buffer + offset, &net_customer_id, sizeof(net_customer_id));
	offset += sizeof(net_customer_id);
	memcpy(buffer + offset, &net_order_number, sizeof(net_order_number));
	offset += sizeof(net_order_number);
	memcpy(buffer + offset, &net_request_type, sizeof(net_request_type));
}

void CustomerRequest::Unmarshal(char *buffer)
{
	int net_customer_id;
	int net_order_number;
	int net_request_type;
	int offset = 0;
	memcpy(&net_customer_id, buffer + offset, sizeof(net_customer_id));
	offset += sizeof(net_customer_id);
	memcpy(&net_order_number, buffer + offset, sizeof(net_order_number));
	offset += sizeof(net_order_number);
	memcpy(&net_request_type, buffer + offset, sizeof(net_request_type));

	customer_id = ntohl(net_customer_id);
	order_number = ntohl(net_order_number);
	request_type = ntohl(net_request_type);
}

bool CustomerRequest::IsValid()
{
	return (customer_id != -1);
}

void CustomerRequest::Print()
{
	std::cout << "id " << customer_id << " ";
	std::cout << "num " << order_number << " ";
	std::cout << "type " << request_type << std::endl;
}

RobotInfo::RobotInfo()
{
	customer_id = -1;
	order_number = -1;
	request_type = -1;
	engineer_id = -1;
	admin_id = -1;
}

void RobotInfo::SetInfo(int id, int number, int type, int engid, int expid)
{
	customer_id = id;
	order_number = number;
	request_type = type;
	engineer_id = engid;
	admin_id = expid;
}

void RobotInfo::CopyOrder(CustomerRequest order)
{
	customer_id = order.GetCustomerId();
	order_number = order.GetOrderNumber();
	request_type = order.GetRequestType();
}
void RobotInfo::SetEngineerId(int id) { engineer_id = id; }
void RobotInfo::SetExpertId(int id) { admin_id = id; }

int RobotInfo::GetCustomerId() { return customer_id; }
int RobotInfo::GetOrderNumber() { return order_number; }
int RobotInfo::GetRequestType() { return request_type; }
int RobotInfo::GetEngineerId() { return engineer_id; }
int RobotInfo::GetExpertId() { return admin_id; }

int RobotInfo::Size()
{
	return sizeof(customer_id) + sizeof(order_number) + sizeof(request_type) + sizeof(engineer_id) + sizeof(admin_id);
}

void RobotInfo::Marshal(char *buffer)
{
	int net_customer_id = htonl(customer_id);
	int net_order_number = htonl(order_number);
	int net_request_type = htonl(request_type);
	int net_engineer_id = htonl(engineer_id);
	int net_admin_id = htonl(admin_id);
	int offset = 0;

	memcpy(buffer + offset, &net_customer_id, sizeof(net_customer_id));
	offset += sizeof(net_customer_id);
	memcpy(buffer + offset, &net_order_number, sizeof(net_order_number));
	offset += sizeof(net_order_number);
	memcpy(buffer + offset, &net_request_type, sizeof(net_request_type));
	offset += sizeof(net_request_type);
	memcpy(buffer + offset, &net_engineer_id, sizeof(net_engineer_id));
	offset += sizeof(net_engineer_id);
	memcpy(buffer + offset, &net_admin_id, sizeof(net_admin_id));
}

void RobotInfo::Unmarshal(char *buffer)
{
	int net_customer_id;
	int net_order_number;
	int net_request_type;
	int net_engineer_id;
	int net_admin_id;
	int offset = 0;

	memcpy(&net_customer_id, buffer + offset, sizeof(net_customer_id));
	offset += sizeof(net_customer_id);
	memcpy(&net_order_number, buffer + offset, sizeof(net_order_number));
	offset += sizeof(net_order_number);
	memcpy(&net_request_type, buffer + offset, sizeof(net_request_type));
	offset += sizeof(net_request_type);
	memcpy(&net_engineer_id, buffer + offset, sizeof(net_engineer_id));
	offset += sizeof(net_engineer_id);
	memcpy(&net_admin_id, buffer + offset, sizeof(net_admin_id));

	customer_id = ntohl(net_customer_id);
	order_number = ntohl(net_order_number);
	request_type = ntohl(net_request_type);
	engineer_id = ntohl(net_engineer_id);
	admin_id = ntohl(net_admin_id);
}

bool RobotInfo::IsValid()
{
	return (customer_id != -1);
}

void RobotInfo::Print()
{
	std::cout << "id " << customer_id << " ";
	std::cout << "num " << order_number << " ";
	std::cout << "type " << request_type << " ";
	std::cout << "engid " << engineer_id << " ";
	std::cout << "adminid " << admin_id << std::endl;
}

CustomerInfo::CustomerInfo()
{
	customer_id = -1;
	last_order = -1;
}

void CustomerInfo::SetRecord(int id, int order_num)
{
	customer_id = id;
	last_order = order_num;
}

int CustomerInfo::GetCustomerId() { return customer_id; }

int CustomerInfo::GetLastOrderNumber() { return last_order; }

int CustomerInfo::Size()
{
	return sizeof(customer_id) + sizeof(last_order);
}

void CustomerInfo::Marshal(char *buffer)
{
	int net_customer_id = htonl(customer_id);
	int net_last_order = htonl(last_order);
	int offset = 0;

	memcpy(buffer + offset, &net_customer_id, sizeof(net_customer_id));
	offset += sizeof(net_customer_id);
	memcpy(buffer + offset, &net_last_order, sizeof(net_last_order));
}

void CustomerInfo::Unmarshal(char *buffer)
{
	int net_customer_id;
	int net_last_order;
	int offset = 0;

	memcpy(&net_customer_id, buffer + offset, sizeof(net_customer_id));
	offset += sizeof(net_customer_id);
	memcpy(&net_last_order, buffer + offset, sizeof(net_last_order));

	customer_id = ntohl(net_customer_id);
	last_order = ntohl(net_last_order);
}

void RoleIdentifier::Marshal(char *buffer)
{
	int r = static_cast<int>(role);
	memcpy(buffer, &r, sizeof(int));
}

void RoleIdentifier::Unmarshal(char *buffer)
{
	int r;
	memcpy(&r, buffer, sizeof(int));
	role = static_cast<Role>(r);
}

ReplicationRequest::ReplicationRequest()
{
	factory_id = -1;
	committed_index = -1;
	last_index = -1;
	op = {-1, -1, -1};
}

void ReplicationRequest::SetMessage(int fid, int cidx, int lidx, MapOp mop)
{
	factory_id = fid;
	committed_index = cidx;
	last_index = lidx;
	op = mop;
}

int ReplicationRequest::GetFactoryId() { return factory_id; }
int ReplicationRequest::GetCommittedIndex() { return committed_index; }
int ReplicationRequest::GetLastIndex() { return last_index; }
MapOp ReplicationRequest::GetMapOp() { return op; }
int ReplicationRequest::Size()
{
	return sizeof(factory_id) + sizeof(committed_index) + sizeof(last_index) + sizeof(op);
}
void ReplicationRequest::Marshal(char *buffer)
{
	int net_factory_id = htonl(factory_id);
	int net_committed_index = htonl(committed_index);
	int net_last_index = htonl(last_index);
	int net_opcode = htonl(op.opcode);
	int net_arg1 = htonl(op.arg1);
	int net_arg2 = htonl(op.arg2);
	int offset = 0;

	memcpy(buffer + offset, &net_factory_id, sizeof(net_factory_id));
	offset += sizeof(net_factory_id);
	memcpy(buffer + offset, &net_committed_index, sizeof(net_committed_index));
	offset += sizeof(net_committed_index);
	memcpy(buffer + offset, &net_last_index, sizeof(net_last_index));
	offset += sizeof(net_last_index);
	memcpy(buffer + offset, &net_opcode, sizeof(net_opcode));
	offset += sizeof(net_opcode);
	memcpy(buffer + offset, &net_arg1, sizeof(net_arg1));
	offset += sizeof(net_arg1);
	memcpy(buffer + offset, &net_arg2, sizeof(net_arg2));

}
void ReplicationRequest::Unmarshal(char *buffer)
{
	int net_factory_id;
	int net_committed_index;
	int net_last_index;
	int net_opcode;
	int net_arg1;
	int net_arg2;
	int offset = 0;

	memcpy(&net_factory_id, buffer + offset, sizeof(net_factory_id));
	offset += sizeof(net_factory_id);
	memcpy(&net_committed_index, buffer + offset, sizeof(net_committed_index));
	offset += sizeof(net_committed_index);
	memcpy(&net_last_index, buffer + offset, sizeof(net_last_index));
	offset += sizeof(net_last_index);
	memcpy(&net_opcode, buffer + offset, sizeof(net_opcode));
	offset += sizeof(net_opcode);
	memcpy(&net_arg1, buffer + offset, sizeof(net_arg1));
	offset += sizeof(net_arg1);
	memcpy(&net_arg2, buffer + offset, sizeof(net_arg2));

	factory_id = ntohl(net_factory_id);
	committed_index = ntohl(net_committed_index);
	last_index = ntohl(net_last_index);
	op.opcode = ntohl(net_opcode);
	op.arg1 = ntohl(net_arg1);
	op.arg2 = ntohl(net_arg2);
	
}

ReplicationResponse::ReplicationResponse()
{
	success = false;
}

void ReplicationResponse::SetResponse(bool succ)
{
	success = succ;
}
bool ReplicationResponse::IsSuccess()
{
	return success;
}

int ReplicationResponse::Size()
{
	return sizeof(int);
}

void ReplicationResponse::Marshal(char *buffer)
{
	int net_success = htonl(success ? 1 : 0);
	memcpy(buffer, &net_success, sizeof(net_success));
}

void ReplicationResponse::Unmarshal(char *buffer)
{
	int net_success;
	memcpy(&net_success, buffer, sizeof(net_success));
	success = (ntohl(net_success) != 0);
}

LatestState::LatestState()
{
	last_index = -2;
	committed_index = -2;
}

void LatestState::SetState(int lidx, int cidx)
{
	last_index = lidx;
	committed_index = cidx;
}

int LatestState::GetLastIndex() { return last_index; }
int LatestState::GetCommittedIndex() { return committed_index; }
int LatestState::Size()
{
	return sizeof(last_index) + sizeof(committed_index);
}

void LatestState::Marshal(char *buffer)
{
	int net_last_index = htonl(last_index);
	int net_committed_index = htonl(committed_index);
	int offset = 0;

	memcpy(buffer + offset, &net_last_index, sizeof(net_last_index));
	offset += sizeof(net_last_index);
	memcpy(buffer + offset, &net_committed_index, sizeof(net_committed_index));
}

void LatestState::Unmarshal(char *buffer)
{
	int net_last_index;
	int net_committed_index;
	int offset = 0;

	memcpy(&net_last_index, buffer + offset, sizeof(net_last_index));
	offset += sizeof(net_last_index);
	memcpy(&net_committed_index, buffer + offset, sizeof(net_committed_index));

	last_index = ntohl(net_last_index);
	committed_index = ntohl(net_committed_index);
}