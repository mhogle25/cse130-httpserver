#include <stdlib.h>

#include "servermanager.h"

// remove 
#include <iostream>
using namespace std;

//NEED TO ACCOUNT FOR THREADING ARGUMENTS
int main(int argc, char** argv) {
    int threadCount;
    bool redundancy = false;
    char* address;
    int port = 80;
    if (argc < 4) {
        // error, not enough arguments, return
    }
    if (argc > 6) {
        // error, too many arguments, return
    }
    if (strcmp(argv[1], "-N") == 0) {
        threadCount = atoi(argv[2]);
        if (strcmp(argv[3],"-r") == 0) {
            redundancy = true;
            address = argv[4];
            if (argc > 5) {
                port = atoi(argv[5]);
            }
        } else {
            address = argv[3];
            if (argc > 4) {
                port = atoi(argv[4]);
            }
        }
    } else {
        address = argv[1];
        if (strcmp(argv[2], "-N") == 0) {
			cout << "enters";
            threadCount = atoi(argv[3]);
            if (argc == 5) {
                if (strcmp(argv[4], "-r") == 0) {
                    redundancy = true;
                }
            }
        } else {
			cout << "enters here";
            port = atoi(argv[2]);
            if (strcmp(argv[3],"-N") == 0) {
                threadCount = atoi(argv[4]);
                if (argc == 6) {
                    if (strcmp(argv[4], "-r") == 0) {
                        redundancy = true;
                    }
                }
            }
        }
    }
    ServerManager serverManager;
    serverManager.Setup(address, port);
    return 0;
}
