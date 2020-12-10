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
#include <errno.h>
#include <time.h>
#include <sys/stat.h>

#include "globalserverinfo.h"
#include "servertools.h"
#define SIZE 16384

class HTTPParse {
private:
	int index;
	char* request;
	int requestLength;
	
	char* requestType;
	int contentLength;
	char* filename;

	int fd[3];
	int bytesUsed[3];
	int correctFileIndex;

	const char* ignore[27];
	
	char* GetWord();
	bool IsValidName(char*);
	int SetupGetRequest();
public:	
	char body[SIZE + 1];
		
	HTTPParse();
	~HTTPParse();
	int ParseRequestHeader(char*);
	int PutAction(int);
	int GetAction();
	int GetRequestType();
	int GetContentLength();
	char* GetFilename();
	int HandleBackups(char*);
	int HandleFolderRecovery(char*);
	bool IsProgramFile(const char*);
	long GetNewestBackup();
};

#endif
