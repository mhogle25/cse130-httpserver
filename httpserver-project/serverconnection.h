#ifndef SERVER_CONNECTION
#define SERVER_CONNECTION

#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <string.h>
#include <err.h>
#include <stdio.h>
#include <fcntl.h>
#include <queue>
#include <pthread.h>
#include <sys/types.h>

#include"servertools.h"
#include "httpparse.h"
#define SIZE 50

class ServerConnection {
public:
	struct ServerConnectionData {
		int index;
		int comm_fd;
		ServerConnection* thisSC;
	};
	void SetupConnection();
	void Init(std::queue<ServerConnection*>*, pthread_mutex_t*, ServerConnectionData*);
	void BeginRecv();	
	static void* toProcess(void*);
	int GetIndex();
private: 
	ServerConnectionData* serverConnectionData;
	bool redundancy;
	std::queue<ServerConnection*>* availableServerConnections;
	pthread_mutex_t* standbyMutex;
	
	char* GenerateMessage(int, int);
};

#endif
