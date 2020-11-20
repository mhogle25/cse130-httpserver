#ifndef GLOBAL_SERVER_INFO
#define GLOBAL_SERVER_INFO

class GlobalServerInfo {
private:
	struct ServerInfo {
		
	};

	static ServerInfo* serverInfos;
public:
	static void InitializeServerInfos(int);
};

#endif
