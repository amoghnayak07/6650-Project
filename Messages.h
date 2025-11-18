#ifndef __MESSAGES_H__
#define __MESSAGES_H__

#include <string>
#include "StateMachine.h"

class CustomerRequest {
private:
	int customer_id;
	int order_number;
	int request_type;

public:
	CustomerRequest();
	void operator = (const CustomerRequest &order) {
		customer_id = order.customer_id;
		order_number = order.order_number;
		request_type = order.request_type;
	}
	int GetCustomerId();
	int GetOrderNumber();
	int GetRequestType();
	void SetOrder(int cid, int order_num, int type);

	void Marshal(char *buffer);
	void Unmarshal(char *buffer);

	int Size();
	bool IsValid();

	void Print();
};

class RobotInfo {
private:
	int customer_id;
	int order_number;
	int request_type;
	int engineer_id;
	int admin_id;

public:
	RobotInfo();
	void operator = (const RobotInfo &info) {
		customer_id = info.customer_id;
		order_number = info.order_number;
		request_type = info.request_type;
		engineer_id = info.engineer_id;
		admin_id = info.admin_id;
	}
	void SetInfo(int cid, int order_num, int type, int engid, int expid);
	void CopyOrder(CustomerRequest order);
	void SetEngineerId(int id);
	void SetExpertId(int id);

	int GetCustomerId();
	int GetOrderNumber();
	int GetRequestType();
	int GetEngineerId();
	int GetExpertId();

	int Size();

	void Marshal(char *buffer);
	void Unmarshal(char *buffer);

	bool IsValid();

	void Print();
};

class CustomerInfo {
private:
	int customer_id;
	int last_order;
public:
	CustomerInfo();
	void operator = (const CustomerInfo &record) {
		customer_id = record.customer_id;
		last_order = record.last_order;
	}
	void SetRecord(int cid, int order_num);
	int GetCustomerId();
	int GetLastOrderNumber();

	int Size();

	void Marshal(char *buffer);
	void Unmarshal(char *buffer);
};

class RoleIdentifier {
private:
public:
	enum Role {
		CLIENT,
		ADMIN,
	} role;
	
	RoleIdentifier() : role(CLIENT) {}
	void SetRole(Role r) { role = r; }
	Role GetRole() { return role; }

	int Size() {
		return sizeof(int);
	}
	void Marshal(char *buffer);
	void Unmarshal(char *buffer);
};

class ReplicationRequest {
private:
	int factory_id;
	int committed_index;
	int last_index;
	MapOp op;
public:
	ReplicationRequest();
	void SetMessage(int fid, int cidx, int lidx, MapOp mop);
	int GetFactoryId();
	int GetCommittedIndex();
	int GetLastIndex();
	MapOp GetMapOp();

	int Size();

	void Marshal(char *buffer);
	void Unmarshal(char *buffer);
};

class ReplicationResponse {
private:
	bool success;
public:
	ReplicationResponse();
	void SetResponse(bool succ);
	bool IsSuccess();

	int Size();
	void Marshal(char *buffer);
	void Unmarshal(char *buffer);
};

class LatestState {
private:
	int last_index;
	int committed_index;
public:
	LatestState();
	void SetState(int lidx, int cidx);
	int GetLastIndex();
	int GetCommittedIndex();

	int Size();

	void Marshal(char *buffer);
	void Unmarshal(char *buffer);
};

#endif // #ifndef __MESSAGES_H__
