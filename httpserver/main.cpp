#include <stdlib.h>

#include "servermanager.h"

int main(int argc, char** argv) {
	if (argc < 2) {
		//Error, not enough arguments, exit
		err(1, "main()");
		return 0;
	}
	
	if (argc > 3) {
		//Error, too many arguments, exit
		err(1, "main()");
		return 0;
	}
	
	char* address = argv[1];
	unsigned short port = 80;
	if (argc > 2) {
		port = atoi(argv[2]);
	}
	
	ServerManager serverManager;
	serverManager.Setup(address, port);
	
	return 0;
}
