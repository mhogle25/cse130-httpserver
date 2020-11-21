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

#include "httpparse.h"

class ServerConnection {
public:
	void Setup(int, bool);
private: 
	int comm_fd;
	
	char* GenerateMessage(int, int);	
};

#endif
