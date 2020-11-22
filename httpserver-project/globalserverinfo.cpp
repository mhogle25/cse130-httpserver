#include "globalserverinfo.h"
#include <map>

int GlobalServerInfo::mutexInfosSize = 0;
bool GlobalServerInfo::redundancy = false;
int GlobalServerInfo::testCounter = 0;

vector<GlobalServerInfo::MutexInfo*> GlobalServerInfo::mutexInfos;
std::map<int, int> GlobalServerInfo::mymap;

void GlobalServerInfo::IncrementCounter() {
	testCounter++;
}

int GlobalServerInfo::GetCounter() {
	return testCounter;
}


bool GlobalServerInfo::AddMutexInfo(char* filename) {
	//std::cout << "inside add mutex info !" << std::endl;
	if (MutexInfoExists(filename)) {
		return false;
	}

	MutexInfo* mutexInfo = new MutexInfo();
	pthread_mutex_init(&mutexInfo->mutex, NULL);
	mutexInfo->filename = filename;
	mutexInfos.push_back(mutexInfo);
	mutexInfosSize++;

	return true;
}

pthread_mutex_t* GlobalServerInfo::GetFileMutex(char* f) {
	for (int i = 0; i < mutexInfosSize; i++) {
		if (strcmp(mutexInfos[i]->filename, f) == 0) {
			//std::cout << mutexInfos[i]->filename << std::endl;
			return &mutexInfos[i]->mutex;
		}
	}

	return NULL;
}

bool GlobalServerInfo::MutexInfoExists(char* f) {
	//std::cout << mutexInfosSize << std::endl;
	//std::cout << "in mutexInfoExists" << std::endl;
	for (int i = 0; i < mutexInfosSize; i++) {
		if (strcmp(mutexInfos[i]->filename, f) == 0) {
			//std::cout << mutexInfos[i]->filename << std::endl;
			return true;
		}
	}
	//std::cout << "end of MutexInfoExists" << std::endl;
	return false;
}

void GlobalServerInfo::RemoveMutexInfo() {
	std::cout << "going to delete" << std::endl;
	for (int i = 0; i < mutexInfosSize; i++) {
		// if (strcmp(mutexInfos[i].filename, f) == 0) {
		std::cout << "inside if statement in removeMutexInfo" << std::endl;
		pthread_mutex_destroy(&mutexInfos[i]->mutex);
		delete mutexInfos[i];
		std::cout << mutexInfos.size() << std::endl;
		mutexInfosSize--;
		std::cout << mutexInfos.size() << std::endl;
		// }
	}
	
	mutexInfos.clear();
}
