#include "servermanager.h"
#include <stdlib.h>

ServerManager::ServerManager() {
	availableServerConnections = new queue<ServerConnection*>();
	listen_fd = 0;
}

ServerManager::~ServerManager() {
	for (int i = 0; i < availableServerConnections->size(); i++) {
		ServerConnection* sc = availableServerConnections->front();
		delete sc;
    	availableServerConnections->pop();
	}
	delete availableServerConnections;
}

/*
  getaddr returns the numerical representation of the address
  identified by *name* as required for an IPv4 address represented
  in a struct sockaddr_in.
*/

unsigned long ServerManager::GetAddress(char *name) {
	unsigned long res;
	struct addrinfo hints;
	struct addrinfo* info;

	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_INET;
	if (getaddrinfo(name, NULL, &hints, &info) != 0 || info == NULL) {
		char msg[] = "getaddrinfo(): address identification error\n";
		write(STDERR_FILENO, msg, strlen(msg));
		exit(1);
	}
	res = ((struct sockaddr_in*) info->ai_addr)->sin_addr.s_addr;
	freeaddrinfo(info);
	return res;
}

void ServerManager::Setup(char* address, unsigned short port, int threadCount, bool redundancy) {
	GlobalServerInfo::redundancy = redundancy;
	
	for (int i = 0; i < threadCount; i++) {
		ServerConnection* serverConnection = new ServerConnection();
		serverConnection->Init(availableServerConnections);
		availableServerConnections->push(serverConnection);
	}	
	
	struct sockaddr_in servaddr;
 	memset(&servaddr, 0, sizeof servaddr);
	
 	listen_fd = socket(AF_INET, SOCK_STREAM, 0);
 	if (listen_fd < 0) {
 		err(1, "socket()");
 	}
 	
 	servaddr.sin_family = AF_INET;
 	servaddr.sin_addr.s_addr = GetAddress(address);
 	servaddr.sin_port = htons(port);
 	if (bind(listen_fd, (struct sockaddr*) &servaddr, sizeof servaddr) < 0) {
 		err(1, "bind()");
 	}
 
 	if (listen(listen_fd, 500) < 0) {
 		err(1, "listen()");
 	}
 	
	while(1) {
		char waiting[] = "waiting for connection\n";
		write(STDOUT_FILENO, waiting, strlen(waiting));
		int comm_fd = accept(listen_fd, NULL, NULL);
		if (comm_fd < 0) {
			warn("accept()");
			continue;
		}
		
		if (availableServerConnections->size() > 0) {
			std::cout << "thread available and will be taken" << std::endl;
			ServerConnection* servCon = availableServerConnections->front();
			availableServerConnections->pop();
			servCon->SetupConnection(comm_fd);
		} else {
			// error: no more threads available
			std::cout << "no threads available" << std::endl;

		}

	}
	GlobalServerInfo::RemoveMutexInfo();
}


