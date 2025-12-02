#include <string.h>
#include <arpa/inet.h>
#include <net/if.h>
#include <netdb.h>
#include <netinet/tcp.h>
#include <sys/types.h>

#include "ClientSocket.h"
#include <iostream>

int ClientSocket::Init(std::string ip, int port) {
	if (is_initialized_) {
		return 0;
	}
	struct sockaddr_in addr;
	fd_ = socket(AF_INET, SOCK_STREAM, 0);
	if (fd_ < 0) {
		return 0;
	}

	memset(&addr, '\0', sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = inet_addr(ip.c_str());
	addr.sin_port = htons(port);

	if ((connect(fd_, (struct sockaddr *) &addr, sizeof(addr))) < 0) {
		return 0;
	}
	// Set reasonable send/recv timeouts so blocking operations fail fast
	struct timeval tv;
	tv.tv_sec = 3;
	tv.tv_usec = 0;
	setsockopt(fd_, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv, sizeof(tv));
	setsockopt(fd_, SOL_SOCKET, SO_SNDTIMEO, (const char*)&tv, sizeof(tv));
	is_initialized_ = true;
	return 1;
}

