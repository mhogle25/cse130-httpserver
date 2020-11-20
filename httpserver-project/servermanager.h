#ifndef SERVER_MANAGER
#define SERVER_MANAGER

#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <string.h>
#include <err.h>
#include <stdio.h>
#include <fcntl.h>

#include "httpparse.h"
#include "globalserverinfo.h"
#include "serverconnection.h"

class ServerManager {
private:
	int listen_fd;
	int comm_fd;
	
	char* GenerateMessage(int, int);
	unsigned long GetAddress(char*);
public:
	ServerManager();
	~ServerManager();
	void Setup(char*, unsigned short);
};

#endif
