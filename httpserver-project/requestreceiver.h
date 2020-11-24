#ifndef REQUEST_RECEIVER
#define REQUEST_RECEIVER

#include "serverconnection.h"
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



class RequestReceiver {
public:
    RequestReceiver();
    ~RequestReceiver();
    void BeginRecv(ServerConnection::ServerConnectionData*);
    char* GetNextValue();
private:
    bool next;
    char* head;
    char* body;

    char* GetWord();
};

#endif