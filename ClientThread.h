#ifndef __CLIENT_THREAD_H__
#define __CLIENT_THREAD_H__

#include "ClientStub.h"
#include "ClientTimer.h"
#include <chrono>
#include <ctime>
#include <string>

class ClientThreadClass {
	int customer_id;
	int order_count;
	int request_type;
	ClientStub stub;
	ClientTimer timer;
	
public:
	ClientThreadClass();
	void SendOrderRequest(std::string ip, int port, int id, int orders, int type);
	void ScanOrders(std::string ip, int port, int max_customer_id, int type);
	void ReadRecordRequest(std::string ip, int port, int id, int orders, int type);

	ClientTimer GetTimer();
};


#endif // end of #ifndef __CLIENT_THREAD_H__
