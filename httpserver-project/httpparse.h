#ifndef HTTP_PARSE
#define HTTP_PARSE

#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <string.h>
#include <err.h>
#include <stdio.h>
#include <fcntl.h>

#include "globalserverinfo.h"

#define SIZE 32768

class HTTPParse {
private:
	int index;
	char* request;
	int requestLength;
	
	char* requestType;
	char* filename;
	int contentLength;
	
	char* GetWord();
	int PutAction();
	int GetAction();
public:	
	char body[SIZE];
		
	HTTPParse();
	~HTTPParse();
	int ParseRequestHeader(char*);
	int ParseRequestBody(char*);
	int GetRequestType();
	int GetContentLength();
};

#endif
