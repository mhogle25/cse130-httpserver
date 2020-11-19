#include "httpparse.h"

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
		return 500;
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
	int fd = open(filename, O_WRONLY | O_CREAT | O_TRUNC);
	
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
