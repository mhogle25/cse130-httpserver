#include "globalserverinfo.h"

GlobalServerInfo::ServerInfo* GlobalServerInfo::serverInfos {
	nullptr
};

void GlobalServerInfo::InitializeServerInfos(int n) {
	if (serverInfos != nullptr) {
		delete[] serverInfos;
	}
	
	serverInfos = new ServerInfo[n];
	serverInfosSize = n;
}

bool GlobalServerInfo::FileBeingUsed(char* f) {
	for (int i = 0; i < serverInfosSize; i++) {
		if (strcmp(serverInfos[i].filename, f) == 0) {
			if (serverInfos.isUsing) {
				return true;
			}
		}
	}
	
	return false;
}
