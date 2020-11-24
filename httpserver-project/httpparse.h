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
#include "servertools.h"

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
	int PutActionRedundancy();
	int GetAction();
	int GetActionRedundancy();
	bool IsValidName(char*);
public:	

	struct fileData {
		int fileSize;
		char* fileContents;
	};
	char* body;
	//char body[SIZE];
		
	HTTPParse();
	~HTTPParse();
	int ParseRequestHeader(char*);
	int ParseRequestBody(char*);
	int GetRequestType();
	int GetContentLength();

	void SetFileToSend(fileData, fileData, fileData, int*);
};

#endif
