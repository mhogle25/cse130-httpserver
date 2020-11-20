#ifndef GLOBAL_SERVER_INFO
#define GLOBAL_SERVER_INFO

#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <string.h>
#include <err.h>
#include <stdio.h>
#include <fcntl.h>

#include "string.h"

class GlobalServerInfo {
private:
	struct ServerInfo {
		char* filename;
		bool isUsing;
	};

	static ServerInfo* serverInfos;
	static int serverInfosSize;
public:
	static void InitializeServerInfos(int);
	static bool FileBeingUsed(char*);
};

#endif
