#include "servermanager.h"
#include <stdlib.h>

ServerManager::ServerManager() {
	availableServerConnections = new std::queue<ServerConnection*>();
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
	
	pthread_mutex_t* servConStandbyMutexes = new pthread_mutex_t[threadCount];
	pthread_t* threads = new pthread_t[threadCount];
	ServerConnection::ServerConnectionData* serverConnectionDatas = new ServerConnection::ServerConnectionData[threadCount];

	for (int i = 0; i < threadCount; i++) {
		serverConnectionDatas[i].thisSC =  new ServerConnection();
		serverConnectionDatas[i].thisSC->Init(availableServerConnections, &servConStandbyMutexes[i], &serverConnectionDatas[i]);
		serverConnectionDatas[i].index = i;

		pthread_mutex_init(&servConStandbyMutexes[i], NULL);
		pthread_mutex_lock(&servConStandbyMutexes[i]);

		pthread_create(&threads[i], NULL, ServerConnection::toProcess, &serverConnectionDatas[i]);

		availableServerConnections->push(serverConnectionDatas[i].thisSC);

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
		std::cout << "[ServerManager] comm_fd when accepting: " << comm_fd << '\n';
		if (comm_fd < 0) {
			warn("accept()");
			continue;
		}


		std::cout << "[ServerManager] Size of Available ServerConnections before pop: " << availableServerConnections->size() << '\n';
		if (availableServerConnections->size() > 0) {
			ServerConnection* servCon = availableServerConnections->front();
			availableServerConnections->pop();
			std::cout << "[ServerManager] Size of Available ServerConnections after pop: " << availableServerConnections->size() << '\n';
			serverConnectionDatas[servCon->GetIndex()].comm_fd = comm_fd;
			std::cout << "[ServerManager] serverConnectionDatas[servCon->GetIndex()].comm_fd: " << serverConnectionDatas[servCon->GetIndex()].comm_fd << '\n';
			std::cout << "[ServerManager] servCon->GetIndex(): " << servCon->GetIndex() << '\n';
			std::cout << "[ServerManager] serverConnectionDatas[servCon->GetIndex()].index: " << serverConnectionDatas[servCon->GetIndex()].index << '\n';


			servCon->SetupConnection();
		} else {
			if (shutdown(comm_fd, SHUT_RDWR) < 0) {
				warn("shutdown()");
			}
			if (close(comm_fd) < 0) {
				warn("warn()");
			}
			// error: no more threads available
			std::cout << "no threads available" << std::endl;

		}

	}

	GlobalServerInfo::RemoveMutexInfo();

	for (int i = 0; i < threadCount; i++) {
		pthread_mutex_destroy(&servConStandbyMutexes[i]);
	}
	delete[] servConStandbyMutexes;

	std::cout << "[ServerManager] About to shut down listen_fd\n";
	if (shutdown(listen_fd, SHUT_RDWR) < 0) {
		warn("shutdown()");
	}
	if (close(listen_fd) < 0) {
		warn("close()");
	}
	std::cout << "[ServerManager] After shutdown listen_fd\n";
}


