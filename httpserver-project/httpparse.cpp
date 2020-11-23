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

int HTTPParse::ParseRequestHeader(char* r, int f) {
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
			messageCode = GetActionRedundancy(f);
		} else {
			messageCode = GetAction(f);
		}

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

int HTTPParse::GetAction(int fdToSend) {
	// NEED: recv() file descriptor so we can send
	// return a 1 if successful and 0 if unsuccessful (500)


	// count the file bytes
	// send this response
	// send the rest of the data
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

	int bytesToSend = CountFileBytes(fd);
	if (bytesToSend == -1) {
		return 403;
	}
	close(fd);
	fd = open(filename, O_RDONLY);
	exists = access(filename, F_OK);
	
	if (fd < 0) {
		if (exists < 0) {
			warn("%s", filename);
			return 404;
		} else {
			warn("%s", filename);
			return 403;
		}

	}

	// send 200 from here
	std::string strLength = std::to_string(bytesToSend);
    char const *bytesToSendChar = strLength.c_str();
    // throw 200
    char throw200[35 + strlen(bytesToSendChar)];
    memset(throw200, 0, sizeof(throw200));
    strncat(throw200, "HTTP/1.1 200 OK\r\nContent-Length: ", 35);
    strncat(throw200, bytesToSendChar, strlen(bytesToSendChar));
    strncat(throw200, "\r\n\r\n", 8);
    int sentGet200 = send(fdToSend, throw200, strlen(throw200), 0);
	if (sentGet200 < 0) {
		std::cout << "couldn't send 200" << std::endl;
	}

	std::cout << "sent 200" << std::endl;

	// lseek
	char buffer[SIZE];
	memset(buffer, 0, sizeof buffer);
	int bytesRead = read(fd, buffer, SIZE);
	if (bytesRead < 0) {
		return 500;
	}
	if (bytesRead == 0) {
		std::cout << "I think we reached end of read already might need lseek" << std::endl;
	}
	int counter = bytesRead;
	int sentSuccessfully = send(fdToSend, buffer, bytesRead, 0);
	while (counter < bytesToSend && bytesRead !=0) {
		bytesRead = read(fd, buffer, SIZE);
		if (bytesRead < 0) {
			return 500;
		}
		if (bytesRead == 0) {
			break;
		}
		// check read
		counter = bytesRead;
		sentSuccessfully = send(fdToSend, buffer, bytesRead, 0);
		if (sentSuccessfully < 0) {
			return 500;
		}
	}
	close(fd);
	std::cout << "returning 1" << std::endl;
	return 1;
}

int HTTPParse::GetActionRedundancy(int fdToSend) {
	// std::cout << "enters get redundancy" << std::endl;

	bool foundFile, sameContent = true;
	bool firstTwoAreSame, secondTwoAreSame, firstAndThirdAreSame = true;
	struct fileData file1, file2, file3;
	
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
		std::cout << "[HTTPParse] filepath:" << filePath << std::endl;
		// open and do stuff
		int fd = open(filePath, O_RDONLY);
		int exists = access(filePath, F_OK); 
			
		if (fd < 0) {
			if (exists < 0) {
				std::cout << "[HTTPParse] 404" << std::endl;
				warn("%s", filename);
				return 404;
			} else {
				std::cout << "[HTTPParse] 403" << std::endl;
				warn("%s", filename);
				return 403;
			}
		}
		
		// count the length of it
		int fileLength = CountFileBytes(fd);
		// int fileLength = 10;
		if (fileLength == -10) {
			return 500;
		}
		// set the lengths
		if (i == 1) {
			file1.fileSize = fileLength;
		} else if (i == 2) {
			file2.fileSize = fileLength;
		} else {
			file3.fileSize = fileLength;
		}
	}
	std::cout << "[HTTPParse] hits here" << std::endl;
	int storedSuccessfully;
	int fileSize = file1.fileSize;
	char* file1Body = new char[fileSize];
	storedSuccessfully = StoreData(file1Body, 1, file1.fileSize);
	if (storedSuccessfully != 1){
		return storedSuccessfully;
	}

	fileSize = file2.fileSize;
	char* file2Body = new char[fileSize];
	storedSuccessfully = StoreData(file2Body, 2, file2.fileSize);
	if (storedSuccessfully != 1){
		return storedSuccessfully;
	}

	fileSize = file3.fileSize;
	char* file3Body = new char[fileSize];
	storedSuccessfully = StoreData(file3Body, 3, file3.fileSize);
	if (storedSuccessfully != 1){
		return storedSuccessfully;
	}
	// if it returns 1 then it was successful
	// THIS IS WRONG THOUGH KEEP TRACK OF MAJORITY

	// set file to send logic, can't put in a function because I need to pass char arrays that change size
	bool equalLengths = (file1.fileSize == file2.fileSize) && (file1.fileSize == file3.fileSize) && (file2.fileSize == file3.fileSize);
	firstTwoAreSame = strcmp(file1Body, file2Body) == 0;
	secondTwoAreSame = strcmp(file2Body, file3Body) == 0;
	firstAndThirdAreSame = strcmp(file1Body, file3Body) == 0;

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

	delete[] file1Body;
	delete[] file2Body;
	delete[] file3Body;
	bool differentFiles = true;
	// if toSend is all 0's then non of the files are the same
	for (int i = 0; i < 3; i++) {
		if (toSend[i] == 1) {
			differentFiles = false;
			if (i==0) {
				// call function to send
				// pass char array and fd
				SendResponseAndBody(fdToSend, file1Body, file1.fileSize);
				break;
			} else if (i==1) {
				SendResponseAndBody(fdToSend, file2Body, file2.fileSize);
				break;
			} else {
				SendResponseAndBody(fdToSend, file3Body, file3.fileSize);
				break;
			}
		}
	}
	if (differentFiles == false) {
		return 1;
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

int HTTPParse::CountFileBytes(int openFD) {
  int fileSize = 0;
  char* buffer[1];
  while (read(openFD, buffer, 1) > 0) {
    fileSize++;
  }
  if(close(openFD) < 0){
	warn("closing %s", "close()");
  }
  return fileSize;
}

int HTTPParse::StoreData(char* fileContents, int folderNum, int length) {
	std::cout << "[HTTPParse] inside store data" << std::endl;
	char filePath[16];
	memset(filePath, 0, sizeof filePath);
	std::string toNum = std::to_string(folderNum);
    char const *folderNumber = toNum.c_str();
	strncpy(filePath, "copy", 4);
	strcat(filePath, folderNumber);
	strncat(filePath, "/", 1);
	strncat(filePath, filename, 10);

	// open and do stuff
	int fd = open(filePath, O_RDONLY);
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
	// return -1 if error, 0 if successful
	char readBytesBuffer[SIZE];
	memset(readBytesBuffer, 0, sizeof readBytesBuffer);
	// read it and copy it into fileContents
	int bytesRead = read(fd, readBytesBuffer, SIZE);
	strncpy(fileContents, readBytesBuffer, bytesRead);
	while (bytesRead > 0) {
		bytesRead = read(fd, readBytesBuffer, SIZE);
		if (bytesRead < 0) {
			std::cout << "error reading" << std::endl;
			return 500;
		}
		strncat(fileContents, readBytesBuffer, bytesRead);
	}
	return 1;
}

void HTTPParse::SendResponseAndBody(int sendfd, char* bodyToSend, int bytesToSend) {

	// need: fd to send to, char buffer to send

	// send 200 from here
	std::string strLength = std::to_string(bytesToSend);
    char const *bytesToSendChar = strLength.c_str();
    // throw 200
    char throw200[35 + strlen(bytesToSendChar)];
    memset(throw200, 0, sizeof(throw200));
    strncat(throw200, "HTTP/1.1 200 OK\r\nContent-Length: ", 35);
    strncat(throw200, bytesToSendChar, strlen(bytesToSendChar));
    strncat(throw200, "\r\n\r\n", 8);
    int sentGet200 = send(sendfd, throw200, strlen(throw200), 0);
	if (sentGet200 < 0) {
		std::cout << "couldn't send 200 REDUNDANCY" << std::endl;
	}

	std::cout << "sent 200 REDUNDANCY" << std::endl;
	int sentBodySuccessfully = send(sendfd, bodyToSend, strlen(bodyToSend), 0);
	// send the char array
	std::cout << "returning 1 REDUNDANCY" << std::endl;
}