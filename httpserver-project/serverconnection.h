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
#define SIZE 32768

class ServerConnection {
public:
	struct ServerConnectionData {
		int index;
		int comm_fd;
		ServerConnection* thisSC;
		~ServerConnectionData() {
			if (thisSC != NULL) 
				delete thisSC;
		}
	};
	void SetupConnection();
	void Init(std::queue<ServerConnection*>*, pthread_mutex_t*, ServerConnectionData*);
	void BeginRecv();	
	static void* toProcess(void*);
	int GetIndex();
private: 
	struct ParseHeaderInfo {
		bool parseHeaderComplete = false;
		bool badRequest = false;
	};
	ServerConnectionData* serverConnectionData;
	std::queue<ServerConnection*>* availableServerConnections;
	pthread_mutex_t* standbyMutex;
	
	char* GenerateMessage(int, int);
	static void* TimeoutBadRequest(void*);
};

#endif
