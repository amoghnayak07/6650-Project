#ifndef __SERVERTHREAD_H__
#define __SERVERTHREAD_H__

#include <condition_variable>
#include <future>
#include <mutex>
#include <queue>
#include <thread>

#include "Messages.h"
#include "ServerSocket.h"
#include "StateMachine.h"
#include "ServerBackupStub.h"
#include "ServerPrimaryStub.h"

struct ExpertRequest
{
	RobotInfo robot;
	std::promise<RobotInfo> prom;
};

class RobotFactory
{
public:
	struct PeerInfo
	{
		int id;
		std::string ip;
		int port;
	};

private:
	struct Peer
	{
		PeerInfo info;
		std::unique_ptr<PrimaryServerStub> primary_stub;
	};
	std::queue<std::unique_ptr<ExpertRequest>> erq;
	std::mutex erq_lock;
	std::condition_variable erq_cv;

	StateMachine sm;

	int last_index;		  // the last index of the smr_log that has data
	int committed_index;  // the last index of the smr_log where the
						  // MapOp of the log entry is committed and
						  // applied to the customer_record
	int primary_id;		  // the production factory id (server id).
						  // initially set to-1.
	const int factory_id; // the id of the factory. This is assigned via
						  // the command line arguments.

	std::vector<Peer> peer_list;
	long unsigned int connected_peers = 0;

	std::mutex replication_lock;

	RobotInfo CreateRegularRobot(CustomerRequest order, int engineer_id);
	RobotInfo CreateSpecialRobot(CustomerRequest order, int engineer_id);
	void handleClientRequest(std::unique_ptr<ServerSocket> socket, int id);
	void handleReplicationRequest(std::unique_ptr<ServerSocket> socket);

	void connectWithPeers();
	void replicateToPeers(MapOp op);
	void replicate(Peer &peer, const int cidx, const int lidx, const MapOp &op);
	void replicateLogToPeer(Peer &peer);

public:
	RobotFactory(int fid, std::vector<PeerInfo> peers);
	void EngineerThread(std::unique_ptr<ServerSocket> socket, int id);
	void AdminThread(int id);
};

#endif // end of #ifndef __SERVERTHREAD_H__
