#include "globalserverinfo.h"

GlobalServerInfo::ServerInfo* GlobalServerInfo::serverInfos {
	nullptr
};

void GlobalServerInfo::InitializeServerInfos(int n) {
	if (serverInfos != nullptr) {
		delete[] serverInfos;
	}
	
	serverInfos = new ServerInfo[n];
}
