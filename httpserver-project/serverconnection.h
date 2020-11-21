#ifndef SERVER_CONNECTION
#define SERVER_CONNECTION

#include "httpparse.h"

class ServerConnection {
public:
	void Setup(int);
private: 
	int comm_fd;	
};

#endif
