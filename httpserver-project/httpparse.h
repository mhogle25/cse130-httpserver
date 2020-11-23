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
	int PutAction(int, int, int);
	int PutActionRedundancy(int, int, int);
	int GetAction(int);
	int GetActionRedundancy(int);
public:	

	struct fileData {
		int fileSize;
		char fileContents[SIZE];
	};
	char body[SIZE];
		
	HTTPParse();
	~HTTPParse();
	int ParseRequestHeader(char*, int);
	int ParseRequestBody(char*, int);
	int GetRequestType();
	int GetContentLength();
	void SetFileToSend(fileData, fileData, fileData, int *);
	bool isValidName(char*);
	int CountFileBytes(int);
	int StoreData(char*, int, int);
	void SendResponseAndBody(int, char*, int);
};

#endif
