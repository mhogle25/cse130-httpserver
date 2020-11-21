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

#include "httpparse.h"

class ServerConnection {
public:
	void SetupConnection(int);
	void Init(std::queue<ServerConnection>*);
private: 
	int comm_fd;
	bool redundancy;
	std::queue<ServerConnection>* availableServerConnections;
	
	char* GenerateMessage(int, int);	
};

#endif
