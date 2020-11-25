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
#define SIZE 50

class HTTPParse {
private:
	int index;
	char* request;
	int requestLength;
	
	char* requestType;
	int contentLength;
	char* filename;

	int fd;
	int bytesUsed;
	
	char* GetWord();
	bool IsValidName(char*);
	int GetFileContentLength();
public:	

	struct fileData {
		int fileSize;
		char* fileContents;
	};

	char body[SIZE];
		
	HTTPParse();
	~HTTPParse();
	int ParseRequestHeader(char*);
	int PutAction(int);
	int GetAction();
	int GetRequestType();
	int GetContentLength();
	char* GetFilename();

	void SetFileToSend(fileData, fileData, fileData, int*);
};

#endif
