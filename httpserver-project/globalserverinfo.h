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
#include <pthread.h>
#include <vector>
#include <queue>

#include <iostream>
#include "string.h"
// #include "serverconnection.h"

class GlobalServerInfo {
private:
	struct MutexInfo {
		char* filename;
		pthread_mutex_t mutex;
	};

	static std::vector<MutexInfo*> mutexInfos;
public:
	static bool redundancy;	
	static int mutexInfosSize;
	
	static bool AddMutexInfo(char*);
	static pthread_mutex_t* GetFileMutex(char*);
	static bool MutexInfoExists(char*);
	static void RemoveMutexInfo();
};

#endif
