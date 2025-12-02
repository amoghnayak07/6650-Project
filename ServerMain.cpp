#include <chrono>
#include <iostream>
#include <mutex>
#include <thread>
#include <vector>

#include "ServerSocket.h"
#include "ServerThread.h"
#include "PersistenceManager.h"

int main(int argc, char *argv[])
{
	// ./server [port #] [unique ID] [# peers] (repeat [ID] [IP] [port #])
	int port;
	int unique_id;
	int num_peers;

	std::vector<RobotFactory::PeerInfo> peer_list;

	if (argc < 4 || argc < 4 + (atoi(argv[3]) * 3))
	{
		std::cout << "not enough arguments" << std::endl;
		std::cout << argv[0] << " [port #] [unique ID]"
				  << " [# peers] (repeat [ID] [IP] [port #])" << std::endl;
		return 0;
	}
	port = atoi(argv[1]);
	unique_id = atoi(argv[2]);
	num_peers = atoi(argv[3]);
	for (int i = 0; i < num_peers; i++)
	{
		RobotFactory::PeerInfo peer;
		peer.id = atoi(argv[4 + i * 3]);
		peer.ip = argv[5 + i * 3];
		peer.port = atoi(argv[6 + i * 3]);
		peer_list.push_back(peer);
	}

	std::cout << "Server started on port " << port << " has unique ID " << unique_id << std::endl;
	std::cout << "Peer count: " << num_peers << std::endl;
	for(const auto& peer : peer_list) {
		std::cout << "Peer ID: " << peer.id << ", IP: " << peer.ip << ", Port: " << peer.port << std::endl;
	}
	
	PersistenceManager pm(unique_id);
	std::vector<MapOp> recovered_log;
	if (!pm.Open()) {
		std::cout << "Warning: Failed to open WAL file." << std::endl;
	} else {
		pm.LoadAll(recovered_log);
		std::cout << "Recovered " << recovered_log.size() << " entries from WAL." << std::endl;
	}

	int engineer_cnt = 0;
	ServerSocket socket;
	
	RobotFactory factory(unique_id, peer_list, &pm, recovered_log);

	std::unique_ptr<ServerSocket> new_socket;
	std::vector<std::thread> thread_vector;

	std::thread admin_thread(&RobotFactory::AdminThread, &factory, 0);
	thread_vector.push_back(std::move(admin_thread));

	if (!socket.Init(port))
	{
		std::cout << "Socket initialization failed" << std::endl;
		return 0;
	}

	while ((new_socket = socket.Accept()))
	{
		std::thread engineer_thread(&RobotFactory::EngineerThread, &factory,
									std::move(new_socket), engineer_cnt++);
		thread_vector.push_back(std::move(engineer_thread));
	}
	return 0;
}