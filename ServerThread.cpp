#include <iostream>
#include <memory>

#include "ServerThread.h"
#include "ServerStub.h"

RobotInfo RobotFactory::CreateRegularRobot(CustomerRequest order, int engineer_id)
{
	RobotInfo robot;
	robot.CopyOrder(order);
	robot.SetEngineerId(engineer_id);
	robot.SetExpertId(-1);
	return robot;
}

RobotInfo RobotFactory::CreateSpecialRobot(CustomerRequest order, int engineer_id)
{
	RobotInfo robot;
	robot.CopyOrder(order);
	robot.SetEngineerId(engineer_id);

	std::promise<RobotInfo> prom;
	std::future<RobotInfo> fut = prom.get_future();

	std::unique_ptr<ExpertRequest> req = std::unique_ptr<ExpertRequest>(new ExpertRequest);
	req->robot = robot;
	req->prom = std::move(prom);

	erq_lock.lock();
	erq.push(std::move(req));
	erq_cv.notify_one();
	erq_lock.unlock();

	robot = fut.get();
	return robot;
}

void RobotFactory::EngineerThread(std::unique_ptr<ServerSocket> socket, int id)
{
	RoleIdentifier idty;
	char buffer[idty.Size()];
	if (socket->Recv(buffer, idty.Size()) <= 0)
	{
		// std::cout << "Sender disconnected before sending Identity." << std::endl;
		return;
	}
	idty.Unmarshal(buffer);
	switch (idty.GetRole())
	{
	case RoleIdentifier::CLIENT:
		handleClientRequest(std::move(socket), id);
		break;
	case RoleIdentifier::ADMIN:
		handleReplicationRequest(std::move(socket));
		break;
	default:
		std::cout << "Undefined identity role from engineer thread: "
				  << idty.GetRole() << std::endl;
	}
}

RobotFactory::RobotFactory(int fid, std::vector<PeerInfo> peers)
	: last_index(-1), committed_index(-1), primary_id(-1), factory_id(fid)
{
	for (const auto &peer : peers)
	{
		Peer p;
		p.info = peer;
		p.primary_stub = nullptr;
		peer_list.push_back(std::move(p));
	}
	
	// Initialize persistence manager and recover from WAL
	persistence_mgr = std::unique_ptr<PersistenceManager>(new PersistenceManager(factory_id));
	persistence_mgr->Open();
	RecoverFromWAL();
}

void RobotFactory::handleReplicationRequest(std::unique_ptr<ServerSocket> socket)
{
	std::unique_ptr<BackupServerStub> stub = std::unique_ptr<BackupServerStub>(new BackupServerStub());
	stub->Init(std::move(socket));

	int thread_primary_id = primary_id;

	LatestState state;
	state.SetState(last_index, committed_index);
	if(!stub->SendLatestState(state)) {
		// std::cout << "Failed to send current state to backup." << std::endl;
		return;
	}

	while (true)
	{
		ReplicationRequest request = stub->ReceiveReplicationInfo();
		if (request.GetFactoryId() == -1)
		{
			if (primary_id == thread_primary_id)
			{
				std::cout << "Primary disconnected." << std::endl;
				primary_id = -1;
			}
			for (auto &peer : peer_list)
			{
				if (peer.info.id == thread_primary_id)
				{
					peer.primary_stub = nullptr;
					connected_peers--;
					break;
				}
			}
			return;
		}
		std::lock_guard<std::mutex> rlock(replication_lock);

		if (request.GetFactoryId() != primary_id)
		{
			primary_id = request.GetFactoryId();
			thread_primary_id = request.GetFactoryId();
			std::cout << "Factory " << primary_id << " is the primary." << std::endl;
		}
		sm.AppendingOperation(request.GetMapOp());
		last_index++;
		sm.ApplyOperation(sm.FetchLog(request.GetCommittedIndex()));
		committed_index = request.GetCommittedIndex();
		ReplicationResponse response;
		response.SetResponse(true);
		stub->SendReplicationInfo(response);
	}
}

void RobotFactory::handleClientRequest(std::unique_ptr<ServerSocket> socket, int id)
{

	int engineer_id = id;
	int request_type;
	CustomerRequest order;
	RobotInfo robot;
	CustomerInfo record;
	ServerStub stub;

	stub.Init(std::move(socket));

	while (true)
	{
		order = stub.ReceiveRequest();
		if (!order.IsValid())
		{
			break;
		}
		request_type = order.GetRequestType();
		switch (request_type)
		{
		case 1:
			robot = CreateSpecialRobot(order, engineer_id);
			stub.ShipRobot(robot);
			break;
		case 2:
			record.SetRecord(order.GetCustomerId(),
							 sm.GetCustomerRecord(order.GetCustomerId()));
			stub.ReturnRecord(record);
			break;
		default:
			std::cout << "Undefined robot type: "
					  << request_type << std::endl;
		}
	}
}

void RobotFactory::RecoverFromWAL()
{
	std::vector<MapOp> recovered_log;
	if (!persistence_mgr->LoadAll(recovered_log))
	{
		std::cerr << "Failed to load WAL for factory " << factory_id << std::endl;
		return;
	}

	if (recovered_log.empty())
	{
		std::cout << "Factory " << factory_id << " - No WAL entries to recover." << std::endl;
		return;
	}

	std::cout << "Factory " << factory_id << " - Recovering from WAL with " << recovered_log.size() << " entries..." << std::endl;

	// Reconstruct the log
	for (const auto& op : recovered_log)
	{
		sm.AppendingOperation(op);
		last_index++;
	}

	// Apply all operations to rebuild customer_record
	if (last_index >= 0)
	{
		sm.ApplyUpTo(last_index);
		committed_index = last_index;
		std::cout << "Factory " << factory_id << " - Recovery complete. Last index: " << last_index << std::endl;
	}
}

void RobotFactory::AdminThread(int id)
{
	std::unique_lock<std::mutex> ul(erq_lock, std::defer_lock);
	while (true)
	{
		ul.lock();

		if (erq.empty())
		{
			erq_cv.wait(ul, [this]
						{ return !erq.empty(); });
		}

		auto req = std::move(erq.front());
		erq.pop();

		ul.unlock();

		std::lock_guard<std::mutex> rlock(replication_lock);
		if (primary_id != factory_id)
		{
			primary_id = factory_id;
			std::cout << "Factory " << factory_id << " became the primary." << std::endl;
			if(committed_index < last_index) {
				sm.ApplyOperation(sm.FetchLog(last_index));
				committed_index = last_index;
			}
		}

		if (connected_peers < peer_list.size()) {
			connectWithPeers();
		}
		
		MapOp op{1, req->robot.GetCustomerId(), sm.GetCustomerRecord(req->robot.GetCustomerId()) + 1};
		sm.AppendingOperation(op);
		last_index++;
		
		// Persist to WAL
		persistence_mgr->AppendEntry(last_index, op);
		
		replicateToPeers(op);
		sm.ApplyOperation(sm.FetchLog(last_index));
		committed_index = last_index;

		req->robot.SetExpertId(id);
		req->prom.set_value(req->robot);
	}
}

void RobotFactory::connectWithPeers()
{
	std::vector<std::thread> threads;
	for (auto &peer : peer_list)
	{
		if (peer.info.id == factory_id)
		{
			continue;
		}
		if (peer.primary_stub == nullptr)
		{
			peer.primary_stub = std::unique_ptr<PrimaryServerStub>(new PrimaryServerStub());
            
			if(peer.primary_stub->Init(peer.info.ip, peer.info.port) == 0)
			{
				// std::cout << "Connection to peer " << peer.info.id << " failed." << std::endl;
				peer.primary_stub = nullptr;
			}
			else
			{
				// std::cout << "Connected to peer " << peer.info.id << "." << std::endl;
				connected_peers++;
				if(last_index >=0) {
					threads.emplace_back(&RobotFactory::replicateLogToPeer, this, std::ref(peer));
				}
			}
		}
	}
	for (auto &t : threads)
	{
		t.join();
	}
}

void RobotFactory::replicateLogToPeer(Peer &peer)
{
	// std::cout << "Replicating log to peer " << peer.info.id << "." << std::endl;
	LatestState peer_state = peer.primary_stub->GetState();
	if(peer_state.GetLastIndex() == -2) {
		// std::cout << "Replication to peer " << peer.info.id << " failed." << std::endl;
		peer.primary_stub = nullptr;
		connected_peers--;
		return;
	}
	int peer_last_index = peer_state.GetLastIndex();
	int peer_committed_index = peer_state.GetCommittedIndex();
	if (peer_last_index == last_index)
	{
		return;
	}
	if (peer_committed_index == -1 && peer_last_index == -1)
	{
		// new peer / recovered peer
		peer_last_index = 0;
	}
	else {
		// existing peer
		peer_committed_index++;
		peer_last_index++;
	}

	for (int Lidx = peer_last_index, Cidx = peer_committed_index; Lidx <= last_index; Lidx++, Cidx++)
	{
		MapOp op = sm.FetchLog(Lidx);
		replicate(peer, Cidx, Lidx, op);
		
	}
}

void RobotFactory::replicateToPeers(MapOp op)
{
	std::vector<std::thread> threads;
	for (auto &peer : peer_list)
	{
		if (peer.info.id == factory_id || peer.primary_stub == nullptr)
		{
			continue;
		}
		threads.emplace_back(&RobotFactory::replicate, this, std::ref(peer) , committed_index, last_index, op);
	}
	for (auto &t : threads)
	{
		t.join();
	}
}

void RobotFactory::replicate(Peer &peer, const int Cidx, const int Lidx, const MapOp &op)
{
	ReplicationRequest msg;
	msg.SetMessage(factory_id, Cidx, Lidx, op);

	ReplicationResponse resp = peer.primary_stub->Replicate(msg);
	if (!resp.IsSuccess())
	{
		// std::cout << "Replication to peer failed." << std::endl;
		peer.primary_stub = nullptr;
		connected_peers--;
	}
}