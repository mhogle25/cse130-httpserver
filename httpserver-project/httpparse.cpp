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

	if (!isValidName(filename)) {
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
		// add mutex ?
		pthread_mutex_t* mutx;
		// std::cout << "outside mutex lock - get" << filename << std::endl;
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
		
		//printf("%s\n", contentLengthHeader);
		//printf("%i\n", contentLength);
		//printf("%i\n", index);
		
		delete[] contentLengthHeader;
		
	}
	// unlock mutex
	return 0;
}

int HTTPParse::ParseRequestBody(char* r, int f) {
	// std::cout << "inside parseReqBody" << pthread_self() << std::endl;
	index = 0;
	request = r;
	requestLength = strlen(request);
	
	pthread_mutex_t* mutx;
	std::cout << GlobalServerInfo::MutexInfoExists(filename) << std::endl;
	if (GlobalServerInfo::MutexInfoExists(filename)) {
		mutx = GlobalServerInfo::GetFileMutex(filename);
	} else {
		// std::cout << "adding mutex - put" << std::endl;
		GlobalServerInfo::AddMutexInfo(filename);
		mutx = GlobalServerInfo::GetFileMutex(filename);
		// return 500 if this is false?
	}
	// std::cout << "thread id: " << pthread_self() << std::endl;
	// std::cout << "outside mutex lock - put" << filename << std::endl;
	pthread_mutex_lock(mutx);
	// std::cout << "inside mutex - put" << std::endl;
	// std::cout << "thread id inside:" <<  pthread_self() << std::endl;	
	// if (contentLength > requestLength) {
	// 	//ERROR, content length is bigger than body, return error code
	// 	return 500;
	// }
	
	strncpy(body, request, contentLength);
	body[contentLength] = '\0';
	
	int messageCode = 500;	
	if (GlobalServerInfo::redundancy) {
		std::cout << "[HTTPParse] calling putAction redundancy" << std::endl;
		messageCode = PutActionRedundancy(requestLength, contentLength, f);
	} else {
		std::cout << "[HTTPParse] calling putAction" << std::endl;
		messageCode = PutAction(requestLength, contentLength, f);
	}
	
	pthread_mutex_unlock(mutx);
	// GlobalServerInfo::RemoveMutexInfo(filename);
	std::cout << "returning messageCode for put" << std::endl; 
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

int HTTPParse::PutAction(int requestLength, int contentLength, int fdescriptor) {
	std::cout << "inside normal put" << std::endl;
	int messageCode = 500;
	int fd = open(filename, O_WRONLY | O_CREAT | O_TRUNC, S_IRWXU);
	
	if (fd < 0) {
		warn("%s", filename);
		messageCode = 403;
	}
	int writtenSuccessful = write(fd, body, strlen(body));
	if (writtenSuccessful != strlen(body)) {
		warn("%s", filename);
		messageCode = 500;
	}

	int counter = requestLength;
	if (counter < contentLength) {
		while (counter < contentLength && writtenSuccessful >= 0) {
			// keep receiving
			char buffer[SIZE];
			memset(buffer, 0, sizeof buffer);
			int received = recv(fdescriptor, buffer, SIZE, 0);
			std::cout << "buffer:" << buffer << std::endl;
			if (received < 0){
                warn("%s", "recv()");
                messageCode = 500;
                break;
            } else if (received == 0){
                warn("%s", "done: recv()");
                break;
            } else {
                counter += received;
                writtenSuccessful = write(fd, buffer, received);
				if (writtenSuccessful < 0) {
					warn("%s", "write()");
					messageCode = 500;
				}
				messageCode = 201;
            }
		}
	}
	// if counter < contentLength
		// do the while stuff

	if (close(fd) < 0) {
		warn("%s", filename);
		messageCode = 500;
	}

	return messageCode;
	
}

int HTTPParse::PutActionRedundancy(int requestLength, int contentLength, int fdescriptor) {
	int fd;
	int messageCode = 500;

	int hasError[3] = {0, 0, 0};
	for (int i = 1; i < 4; i++) {
		std::cout << "value of i: " << i << std::endl;
		char filePath[16];
		memset(filePath, 0, sizeof filePath);
		std::string toNum = std::to_string(i);
		char const *folderNumber = toNum.c_str();
		strncpy(filePath, "copy", 4);
		strcat(filePath, folderNumber);
		strncat(filePath, "/", 1);
		strncat(filePath, filename, 10);

		if (i == 1) {
			//"copy1/Small12345"
			fd = open(filePath, O_WRONLY | O_CREAT | O_TRUNC, S_IRWXU);
			if (fd < 0) {
				warn("%s", filename);
				hasError[i-1] = 1;
			}
				
			int writtenSuccessful = write(fd, body, strlen(body));
			if (writtenSuccessful != strlen(body)) {
				warn("%s", filename);
				hasError[i-1] = 1;
			}

			int counter = requestLength;
			if (counter < contentLength) {
				while (counter < contentLength && writtenSuccessful >= 0) {
					// keep receiving
					char buffer[SIZE];
					memset(buffer, 0, sizeof buffer);
					int received = recv(fdescriptor, buffer, SIZE, 0);
					if (received < 0){
						warn("%s", "recv()");
						hasError[i-1] = 1;
						break;
					} else if (received == 0){
						warn("%s", "recv()");
						break;
					} else {
						counter += received;
						writtenSuccessful = write(fd, buffer, received);
						if (writtenSuccessful < 0) {
							warn("%s", "write()");
							hasError[i-1] = 1;

						}
					}
				}
				std::cout << "out of loop" << std::endl;
			}
		} else {
			// get copy1 file
			char copy1filePath[16];
			memset(copy1filePath, 0, sizeof copy1filePath);
			std::string toNum = std::to_string(1);
			char const *folderNumber = toNum.c_str();
			strncpy(copy1filePath, "copy", 4);
			strcat(copy1filePath, folderNumber);
			strncat(copy1filePath, "/", 1);
			strncat(copy1filePath, filename, 10);
			// open copy1 file
			int firstFD = open(copy1filePath, O_RDONLY);
			// open other file
			fd = open(filePath, O_WRONLY | O_CREAT | O_TRUNC, S_IRWXU);
			char buffer[SIZE];
			memset(buffer, 0, sizeof buffer);
			int bytesRead = read(firstFD, buffer, SIZE);
                if (bytesRead < 0){
                    warn("%s", "read()");
					hasError[i-1] = 1;
                } else if (bytesRead > 0) {
                    printf("greater than 0");
                    write(fd, buffer, bytesRead);
                    while (bytesRead > 0) {
                        bytesRead = read(firstFD, buffer, SIZE);
                        if (bytesRead < 0 ){
                            warn("%s", filename);
                            hasError[i-1] = 1;
							break;
                        }
                        write(fd, buffer, bytesRead);
                    }
                } else {
                    printf("is0");
                    write(fd, buffer, bytesRead);
                }
				close(firstFD);
		}
		
			
		if (close(fd) < 0) {
			warn("%s", filename);
			hasError[i] = 1;
		}
	}
	std::cout << "out of condition" << std::endl;
	// if they're all 0's send 201
	int numBadFiles = 0;
	for (int i = 0; i < 3; i++) {
		if (hasError[i] == 1){
			numBadFiles++;
		}
	}
	if (numBadFiles == 1) {
		std::cout << "numbadfiles == 1" << std::endl;
		return 500; // ask TA
	}
	std::cout << "returning" << std::endl;
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

bool HTTPParse::isValidName(char* filename) {
	for (unsigned long i = 0; i < strlen(filename); i++) {
        if (isalpha(filename[i]) || isdigit(filename[i])){
            int num = (int)filename[i] - '0'; // look at ascii chart otherwise 0 turns into 48
            if (isdigit(filename[i])) {
                if (num > 9 || num < 0) {
                    return false;
                }
            }
        } else {
        	return false;
    	}
    }
    return true;
}