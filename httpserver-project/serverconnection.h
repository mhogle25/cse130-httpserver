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

#include "httpparse.h"

class ServerConnection {
public:
	void SetupConnection(int);
	void Init(std::queue<ServerConnection*>*, pthread_mutex_t*);
	void doStuff();	
	static void* toProcess(void*);
private: 
	int comm_fd;
	bool redundancy;
	std::queue<ServerConnection*>* availableServerConnections;
	pthread_mutex_t* standbyMutex;
	
	char* GenerateMessage(int, int);
};

#endif
