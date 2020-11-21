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
}

HTTPParse::~HTTPParse() {
	delete[] requestType;
	delete[] filename;
}

int HTTPParse::ParseRequestHeader(char* r) {
	index = 0;
	request = r;
	requestLength = strlen(request);
	
	requestType = GetWord();
	
	//printf("%s\n", requestType);
	
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
	
	//printf("%s\n", filename);
	
	char* httpTitle = GetWord();
	
	if (strcmp(httpTitle, "HTTP/1.1") != 0) {
		//ERROR, not an HTTP request, return error code
		return 400;
	}
	
	delete[] httpTitle;
	
	//printf("%s\n", httpTitle);
	
	if (GetRequestType() == -1) {
		//ERROR, not an HTTP request, return error code
		return 500;
	}
	
	if (GetRequestType() == 0) {	//GET
		if (GlobalServerInfo::redundancy) {
			return GetActionRedundancy();
		}
		return GetAction();
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
		
		//printf("%s\n", contentLengthHeader);
		//printf("%i\n", contentLength);
		//printf("%i\n", index);
		
		delete[] contentLengthHeader;
		
	}
	
	return 0;
}

int HTTPParse::ParseRequestBody(char* r) {
	index = 0;
	request = r;
	requestLength = strlen(request);
	
	if (contentLength > requestLength) {
		//ERROR, content length is bigger than body, return error code
		return 500;
	}
	
	strncpy(body, request, contentLength);
	body[contentLength] = '\0';
	
	if (GlobalServerInfo::redundancy) {
		return PutActionRedundancy();
	}
	return PutAction();
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
	std::cout << "inside normal put" << std::endl;
	int fd = open(filename, O_WRONLY | O_CREAT | O_TRUNC, S_IRWXU);
	
	if (fd < 0) {
		warn("%s", filename);
		return 403;
	}
	
	if (write(fd, body, strlen(body)) != strlen(body)) {
		warn("%s", filename);
		return 500;
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
		strcat(filePath, folderNumber);
		strncat(filePath, "/", 1);
		strncat(filePath, filename, 10);

		//"copy1/Small12345"
		fd = open(filePath, O_WRONLY | O_CREAT | O_TRUNC, S_IRWXU);
		if (fd < 0) {
			warn("%s", filename);
			hasError[i] = 1;
		}
			
		if (write(fd, body, strlen(body)) != strlen(body)) {
			warn("%s", filename);
			hasError[i] = 1;
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
	body[0] = '\0';
	while (read(fd, buffer, 1) > 0) {
		buffer[1] = '\0';
		strncat(body, buffer, 1);
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
	// std::cout << "enters get redundancy" << std::endl;

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
		strcat(filePath, folderNumber);
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
			body[0] = '\0';
			while (read(fd, buffer, 1) > 0) {
				buffer[1] = '\0';
				strncat(body, buffer, 1);
			} 
			
			contentLength = strlen(body);

		if (i == 1) {
			// std::cout << "saving to first" << std::endl;
			file1.fileSize = contentLength;
			strcpy(file1.fileContents, body);
		} else if (i == 2) {
			file2.fileSize = contentLength;
			strcpy(file2.fileContents, body);
		} else {
			file3.fileSize = contentLength;
			strcpy(file3.fileContents, body);
		}

		//printf("%s\n", body);
		//printf("%lu\n", strlen(body));
		//printf("%i\n", contentLength);
		
		if (close(fd) < 0) {
			warn("%s", filename);
			return 500;
		}
	}

	SetFileToSend(file1, file2, file3, toSend);
	bool differentFiles = true;
	// if toSend is all 0's then non of the files are the same
	for (int i = 0; i < 3; i++) {
		// std::cout << "tosend" << std::endl;
		// std::cout << i << std::endl;
		// std::cout << toSend[i] << std::endl;
		if (toSend[i] == 1) {
			differentFiles = false;
			if (i==0) {
				// std::cout << "enters case 1" << std::endl;
				contentLength = file1.fileSize;
				// clear body probably
				strcpy(body, file1.fileContents);
				break;
			} else if (i==1) {
				// std::cout << "enters case 2" << std::endl;
				contentLength = file2.fileSize;
				// clear body probably
				strcpy(body, file2.fileContents);
				break;
			} else {
				// std::cout << "enters case 3" << std::endl;
				contentLength = file3.fileSize;
				// clear body probably
				strcpy(body, file3.fileContents);
				break;
			}
		}
	}
	if (differentFiles == false) {
		// std::cout << "check contentlength and buffer" << std::endl;
		// std::cout << contentLength << std::endl;
		// std::cout << body << std::endl;

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
	//printf("%i\n", contentLength);
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
			// std::cout << "first two are same" << std::endl;
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
