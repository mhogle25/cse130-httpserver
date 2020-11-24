#include "httpparse.h"
#include <sys/types.h>
#include <dirent.h>
#include <libgen.h>
#include <string>
#include <iostream>

HTTPParse::HTTPParse() {
	index = 0;
	requestLength = 0;
	contentLength = -1;
	requestType = NULL;
	filename = NULL;
	contentLength = 0;
	body = new char[1];
	body[0] = '\0';
}

HTTPParse::~HTTPParse() {
	delete[] requestType;
	delete[] filename;
	if (body != NULL) {
		delete[] body;
	}
}

int HTTPParse::ParseRequestHeader(char* r) {
	index = 0;
	request = r;
	requestLength = strlen(request);
	
	requestType = GetWord();
	
	filename = GetWord();
	
	if (filename[0] == '/') {
		memmove(filename, filename+1, strlen(filename));
	} else {
		//ERROR, not an HTTP request, return error code
		return 400;
	}
	
	if (strlen(filename) != 10) {
		return 400;
	}
	
	char* httpTitle = GetWord();
	
	if (strcmp(httpTitle, "HTTP/1.1") != 0) {
		//ERROR, not an HTTP request, return error code
		return 400;
	}
	
	delete[] httpTitle;
	
	if (GetRequestType() == -1) {
		//ERROR, not an HTTP request, return error code
		return 500;
	}
	
	if (GetRequestType() == 0) {	//GET
		// add mutex ?
		pthread_mutex_t* mutx;
		//std::cout << "outside mutex lock - get" << filename << std::endl;
		if (GlobalServerInfo::MutexInfoExists(filename)) {
			mutx = GlobalServerInfo::GetFileMutex(filename);
		} else {
			//std::cout << "adding mutex - get" << std::endl;
			GlobalServerInfo::AddMutexInfo(filename);
			mutx = GlobalServerInfo::GetFileMutex(filename);
			// return 500 if this is false?
		}
		pthread_mutex_lock(mutx);
		
		int messageCode = 500;
		if (GlobalServerInfo::redundancy) {
			messageCode = GetActionRedundancy();
		}
		messageCode = GetAction();

		pthread_mutex_unlock(mutx);
		// GlobalServerInfo::RemoveMutexInfo(filename);
		return messageCode;
	}
	
	if (GetRequestType() == 1) {	//PUT
		
		char* contentLengthHeader = new char[0];
		
		while (strcmp(contentLengthHeader, "Content-Length:") != 0 && index < requestLength) {
			delete[] contentLengthHeader;
			contentLengthHeader = GetWord();
		}
		
		
		if (index < requestLength) {
			contentLength = atoi(GetWord());
		}
		
		delete[] contentLengthHeader;
		
	}
	// unlock mutex
	return 0;
}

int HTTPParse::ParseRequestBody(char* r) {
	//std::cout << "inside parseReqBody" << pthread_self() << std::endl;
	index = 0;
	request = r;
	requestLength = strlen(request);
	
	pthread_mutex_t* mutx;
	//std::cout << GlobalServerInfo::MutexInfoExists(filename) << std::endl;
	if (GlobalServerInfo::MutexInfoExists(filename)) {
		mutx = GlobalServerInfo::GetFileMutex(filename);
	} else {
		//std::cout << "adding mutex - put" << std::endl;
		GlobalServerInfo::AddMutexInfo(filename);
		mutx = GlobalServerInfo::GetFileMutex(filename);
		// return 500 if this is false?
	}
	//std::cout << "thread id: " << pthread_self() << std::endl;
	//std::cout << "outside mutex lock - put" << filename << std::endl;
	pthread_mutex_lock(mutx);
	//std::cout << "inside mutex - put" << std::endl;
	//std::cout << "thread id inside:" <<  pthread_self() << std::endl;	
	if (contentLength > requestLength) {
		//ERROR, content length is bigger than body, return error code
		return 500;
	}
	
	GetSubstringFront(body, request, contentLength);
	
	int messageCode = 500;	
	if (GlobalServerInfo::redundancy) {
		messageCode = PutActionRedundancy();
	}
	messageCode = PutAction();
	
	pthread_mutex_unlock(mutx);
	return messageCode;
}

int HTTPParse::GetRequestType() {
	if (requestType == NULL) {
		return -1;
	}

	if (strcmp(requestType, "GET") == 0) {
		return 0;
	}
	
	if (strcmp(requestType, "PUT") == 0) {
		return 1;
	}
	
	return -1;
}

int HTTPParse::PutAction() {
	int fd = open(filename, O_WRONLY | O_CREAT | O_TRUNC, S_IRWXU);
	
	if (fd < 0) {
		warn("%s", filename);
		return 403;
	}
	
	for (int i = 0; i < strlen(body); i++) {
		char buf[1];
		buf[0] = body[i];
		if (write(fd, buf, 1) < 1) {
			warn("%s", filename);
			return 500;
		}
	}
	
	if (close(fd) < 0) {
		warn("%s", filename);
		return 500;
	}
	
	return 201;
	
}

int HTTPParse::PutActionRedundancy() {
	int fd;
	
	int hasError[3] = {0, 0, 0};
	for (int i = 1; i < 4; i++) {
		char filePath[16];
		memset(filePath, 0, sizeof filePath);
		std::string toNum = std::to_string(i);
    	char const *folderNumber = toNum.c_str();
		strncpy(filePath, "copy", 4);
		strncat(filePath, folderNumber, strlen(folderNumber));
		strncat(filePath, "/", 1);
		strncat(filePath, filename, 10);

		//"copy1/Small12345"
		fd = open(filePath, O_WRONLY | O_CREAT | O_TRUNC, S_IRWXU);
		if (fd < 0) {
			warn("%s", filename);
			hasError[i] = 1;
		}
			
		for (int i = 0; i < strlen(body); i++) {
			char buf[1];
			buf[0] = body[i];
			if (write(fd, buf, 1) < 1) {
				warn("%s", filename);
				return 500;
			}
		}
			
		if (close(fd) < 0) {
			warn("%s", filename);
			hasError[i] = 1;
		}
	}
	// if they're all 0's send 201
	int numBadFiles = 0;
	for (int i = 0; i < 3; i++) {
		if (hasError[i] == 1){
			numBadFiles++;
		}
	}
	if (numBadFiles == 1) {
		return 500; // ask TA
	}
	return 201;
}

int HTTPParse::GetAction() {
	int fd = open(filename, O_RDONLY);
	
	int exists = access(filename, F_OK);
	
	if (fd < 0) {
		if (exists < 0) {
			warn("%s", filename);
			return 404;
		} else {
			warn("%s", filename);
			return 403;
		}

	}
	
	char buffer[8];
	while (read(fd, buffer, 1) > 0) {
		buffer[1] = '\0';
		body = strcat(body, buffer);
	} 
	
	contentLength = strlen(body);
	
	//printf("%s\n", body);
	//printf("%lu\n", strlen(body));
	//printf("%i\n", contentLength);
	
	if (close(fd) < 0) {
		warn("%s", filename);
		return 500;
	}
	
	return 200;
}

int HTTPParse::GetActionRedundancy() {

	bool foundFile, sameContent = true;
	bool firstTwoAreSame, secondTwoAreSame, firstAndThirdAreSame = true;
	struct fileData file1, file2, file3;
	int fd;
	
	int toSend[3] = {0, 0, 0};
	for (int i = 1; i < 4; i++) {
		// set paths
		char filePath[16];
		memset(filePath, 0, sizeof filePath);
		std::string toNum = std::to_string(i);
    	char const *folderNumber = toNum.c_str();
		strncpy(filePath, "copy", 4);
		strncat(filePath, folderNumber, strlen(folderNumber));
		strncat(filePath, "/", 1);
		strncat(filePath, filename, 10);

		// open and do stuff
		fd = open(filePath, O_RDONLY);
		int exists = access(filePath, F_OK);
			
		if (fd < 0) {
			if (exists < 0) {
				warn("%s", filename);
				return 404;
			} else {
				warn("%s", filename);
				return 403;
			}
		}
			
			char buffer[8];
			while (read(fd, buffer, 1) > 0) {
				buffer[1] = '\0';
				body = strcat(body, buffer);
			} 
			
			contentLength = strlen(body);

		if (i == 1) {
			file1.fileSize = contentLength;
			GetSubstringFront(file1.fileContents, body, strlen(body));
		} else if (i == 2) {
			file2.fileSize = contentLength;
			GetSubstringFront(file2.fileContents, body, strlen(body));
		} else {
			file3.fileSize = contentLength;
			GetSubstringFront(file3.fileContents, body, strlen(body));
		}
		
		if (close(fd) < 0) {
			warn("%s", filename);
			return 500;
		}
	}

	SetFileToSend(file1, file2, file3, toSend);
	bool differentFiles = true;
	// if toSend is all 0's then non of the files are the same
	for (int i = 0; i < 3; i++) {
		if (toSend[i] == 1) {
			differentFiles = false;
			if (i==0) {
				contentLength = file1.fileSize;
				// clear body probably
				GetSubstringFront(body, file1.fileContents, strlen(file1.fileContents));
				break;
			} else if (i==1) {
				contentLength = file2.fileSize;
				// clear body probably
				GetSubstringFront(body, file2.fileContents, strlen(file1.fileContents));
				break;
			} else {
				contentLength = file3.fileSize;
				// clear body probably
				GetSubstringFront(body, file3.fileContents, strlen(file1.fileContents));
				break;
			}
		}
	}
	if (differentFiles == false) {

		return 200;
	}
	return 500;
}

//Function will return the next substring before a space or newline, starting at index 0
//Index tracked by HttpParse
char* HTTPParse::GetWord() {
	char* word = new char[1];
	word[0] = '\0';

	if (index >= requestLength) {
		return word;
	}

	for ( ; index < requestLength; index++) {
		if (request[index] == '\r') {
			continue;
		}
		
		if (request[index] == '\n' || request[index] == ' ') {
			index++;
			break;
		}
		
		char character[2];
		character[0] = request[index];
		character[1] = '\0';
		
		word = strcat(word, character);
	}

	return word;
}

int HTTPParse::GetContentLength() {
	return contentLength;
}

void HTTPParse::SetFileToSend(fileData f1, fileData f2, fileData f3, int *toSend) {
	bool equalLengths = (f1.fileSize == f2.fileSize) && (f1.fileSize == f3.fileSize) && (f2.fileSize == f3.fileSize);
	bool firstTwoAreSame = strcmp(f1.fileContents, f2.fileContents) == 0;
	bool secondTwoAreSame = strcmp(f2.fileContents, f3.fileContents) == 0;
	bool firstAndThirdAreSame = strcmp(f1.fileContents, f3.fileContents) == 0;

	if (firstTwoAreSame && secondTwoAreSame && firstAndThirdAreSame) {
		// send whichever
		toSend[0] = 1;
	} else {
		// they're not the same contents
		if (firstTwoAreSame) {
			// return one of the two
			toSend[0] = 1;
		} else if (secondTwoAreSame) {
			// return one of the two
			toSend[1] = 1;
		} else if (firstAndThirdAreSame) {
			// return one of the two
			toSend[2] = 1;
		}
	}
}

void HTTPParse::GetSubstringFront(char*& destination, char* source, int bytes) {
	if (bytes > strlen(source)) {
		warn("GetSubstringFront()");
		return;
	}

	if (destination != NULL) {
		delete[] destination;
	}

	destination = new char[bytes + 1];
	for (int i = 0; i < bytes; i++) {
		destination[i] = source[i];
	}

	destination[bytes] = '\0';
}
