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
	for (int i = 0; i < 3; i++) {
		fd[i] = -1;
		bytesUsed[i] = 0;
	}
	correctFileIndex = 0;

	ignore[0] = "DESIGN.pdf";
	ignore[1] = "globalserverinfo.cpp";
	ignore[2] = "globalserverinfo.h";
	ignore[3] = "globalserverinfo.o";
	ignore[4] = "httpparse.cpp";
	ignore[5] = "httpparse.h";
	ignore[6] = "httpparse.o";
	ignore[7] = "httpserver";
	ignore[8] = "main.cpp";
	ignore[9] = "main.o";
	ignore[10] = "Makefile";
	ignore[11] = "README.md";
	ignore[12] = "serverconnection.cpp";
	ignore[13] = "serverconnection.h";
	ignore[14] = "serverconnection.o";
	ignore[15] = "servermanager.cpp";
	ignore[16] = "servermanager.h";
	ignore[17] = "servermanager.o";
	ignore[18] = "servertools.cpp";
	ignore[19] = "servertools.h";
	ignore[20] = "servertools.o";
	ignore[21] = ".";
	ignore[22] = "..";
	ignore[23] = "client";
	ignore[24] = "copy1";
	ignore[25] = "copy2";
	ignore[26] = "copy3";
}

HTTPParse::~HTTPParse() {
	delete[] requestType;
	delete[] filename;
}

int HTTPParse::ParseRequestHeader(char* r) {
	std::cout << "[HTTPParse] header: " << r << '\n';
	index = 0;
	request = r;
	requestLength = strlen(request);
	
	requestType = GetWord();
	std::cout << "[HTTPParse] request type: " << requestType << '\n';

	filename = GetWord();
	std::cout << "[HTTPParse] filename: " << filename << '\n';
	if (filename[0] == '/') {
		memmove(filename, filename+1, strlen(filename));
	} else {
		//ERROR, not an HTTP request, return error code
		return 400;
	}
	std::cout << "[HTTPParse] without slash: " << filename << '\n';	
	bool isValidFunctionality = (strcmp(filename, "r") == 0 || strcmp(filename, "b") == 0 || strcmp(filename, "l") == 0);
	std::cout << "[HTTPParse] isValidFunctionality " << isValidFunctionality<< '\n';
	if (strlen(filename) != 10 &&  strlen(filename) != 1) {
		std::cout << "size of filename" << strlen(filename) << '\n';
		return 400;
	}
	
	if (!isValidFunctionality) {
		return 400;
	}
	if (!IsValidName(filename)) {
		return 400;
	}
	
	char* httpTitle = GetWord();
	std::cout << "[HTTPParse] title: " << httpTitle << '\n';

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

		int messageCode;
		if (strcmp(filename, "r") == 0) {
			// call recovery function
			std::cout << "[HTTPParse] inside r if statement " << r << '\n';
		} else if (strcmp(filename, "b") == 0) {
			std::cout << "[HTTPParse] inside b if statement " << r << '\n';
			messageCode = HandleBackups(filename);
		} else if (strcmp(filename, "l") == 0) {
			// call list function
		} else {
 			messageCode = SetupGetRequest();
		}

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
		} else {
			contentLength = -1;	//This case is for when no PUT content length is sent
		}
		
		delete[] contentLengthHeader;
		
	}
	// unlock mutex
	return 0;
}

int HTTPParse::PutAction(int s) {
	int fileCount = 1;
	if (GlobalServerInfo::redundancy) {
		fileCount = 3;
	}
	for (int i = 0; i < fileCount; i++) {
		if (fd[i] < 0) {
			char* filepath;
			char prefix[17] = "copy1/";
			prefix[4] = '1' + i;
			if (GlobalServerInfo::redundancy) {
				filepath = strncat(prefix, filename, 10);
			} else {
				filepath = filename;
			}
			fd[i] = open(filepath, O_WRONLY | O_CREAT | O_TRUNC, S_IRWXU);
			if (fd[i] < 0) {
				warn("%s", filepath);
				return 403;
			}
		}

		int n = write(fd[i], body, s);
		bytesUsed[i] += n;

		if (bytesUsed[i] >= contentLength) {
			if (close(fd[i]) < 0) {
				warn("%s", filename);
				return 500;
			}
		}
	}
	return 201;
}

int HTTPParse::GetAction() {
	if (fd[correctFileIndex] < 0) {
		char* filepath;
		char prefix[17] = "copy1/";
		prefix[4] = '1' + correctFileIndex;
		if (GlobalServerInfo::redundancy) {
			filepath = strncat(prefix, filename, 10);
		} else {
			filepath = filename;
		}
		fd[correctFileIndex] = open(filepath, O_RDONLY);
	
		if (fd[correctFileIndex] < 0) {
			warn("%s", filepath);
			return -1;
		}
	}

	int n = read(fd[correctFileIndex], body, SIZE);
	bytesUsed[correctFileIndex] += n;

	if (bytesUsed[correctFileIndex] >= contentLength) {
		if (close(fd[correctFileIndex]) < 0) {
			warn("%s", filename);
			return -1;
		}
	}

	return n;
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
		
		ServerTools::AppendChar(word, character[0]);
	}

	return word;
}

int HTTPParse::SetupGetRequest() {
	pthread_mutex_t* mutx;
	if (GlobalServerInfo::MutexInfoExists(filename)) {
		mutx = GlobalServerInfo::GetFileMutex(filename);
	} else {
		GlobalServerInfo::AddMutexInfo(filename);
		mutx = GlobalServerInfo::GetFileMutex(filename);
	}
	pthread_mutex_lock(mutx);	//critical region

	int fileDescriptor[3];
	int errorCode[3];
	bool fileComplete[3];
	int bytesRead[3];
	int n[3];

	for (int i = 0; i < 3; i++) {
		fileDescriptor[i] = -1;
		errorCode[i] = -1;
		fileComplete[i] = false;
		bytesRead[i] = 0;
		n[i] = 0;
	}

	int fileCount = 1;
	bool badFile[3];

	if (GlobalServerInfo::redundancy) {
		fileCount = 3;
		badFile[0] = false;
		badFile[2] = false;
		badFile[3] = false;
	}

	char buffer[3][SIZE + 1];

	while (true) {
		int errorCount = 0;
		for (int i = 0; i < fileCount; i++) {
			if (fileComplete[i] == false) {
				if (fileDescriptor[i] < 0){
					char* filepath;
					char prefix[17] = "copy1/";
					prefix[4] = '1' + i;
					if (GlobalServerInfo::redundancy) {
						filepath = strncat(prefix, filename, 10);
					} else {
						filepath = filename;
					}
					fileDescriptor[i] = open(filepath, O_RDONLY);
					if (fileDescriptor[i] < 0) {
						if (errno == EACCES) {
							warn("%s", filepath);
							errorCode[i] = 403;
						} else {
							warn("%s", filepath);
							errorCode[i] = 404;
						}
						errorCount++;
						fileComplete[i] = true;

						if (GlobalServerInfo::redundancy) {
							if (errorCount > 1) {
								int err1 = -1;
								int err2 = -1;
								for(int j = 0; j < fileCount; j++) {
									if (errorCode[j] != -1) {
										if (err1 != -1) {
											err2 = errorCode[j];
										} else {
											err1 = errorCode[j];
										}
									}
									if (err1 != -1 && err2 != -1) {
										break;
									}
								}
								if (err1 == err2) {
									return err1;
								} else {
									return 500;
								}
							}
						} else {
							if (errorCount > 0) {
								return errorCode[0];
							}
						}

						continue;
					}
				}
				
				n[i] = read(fileDescriptor[i], buffer[i], SIZE);
				buffer[i][n[i]] = '\0';
				bytesRead[i] += n[i];

				if (n[i] < SIZE) {
					if (close(fileDescriptor[i]) < 0) {
						warn("%s", filename);
						return 500;
					}
					fileDescriptor[i] = -1;
					fileComplete[i] = true;
				}
			}
		}

		if (GlobalServerInfo::redundancy) {

			bool comp1and2;
			if (!(badFile[0] || badFile[1])) {
				comp1and2 = strcmp(buffer[0], buffer[1]) != 0;
			}
			bool comp1and3;
			if (!(badFile[0] || badFile[2])) {
				comp1and3 = strcmp(buffer[0], buffer[2]) != 0;
			}
			bool comp2and3;
			if (!(badFile[1] || badFile[2])) {
				comp2and3 = strcmp(buffer[1], buffer[2]) != 0;
			}

			if (!(badFile[0] || badFile[1] || badFile[2])) {
				if (comp1and2) {
					if (comp1and3){
						if (comp2and3) {
							for (int i = 0; i < 3; i++) {
								if (close(fileDescriptor[i]) < 0) {
									warn("%s", filename);
								}
								return 500;
							}
						} else {
							if (fileDescriptor[0] >= 0) {
								if (close(fileDescriptor[0]) < 0) {
									warn("%s", filename);
									return 500;
								}
								fileDescriptor[0] = -1;
							}
							badFile[0] = true;
							fileComplete[0] = true;
							correctFileIndex = 1;
						}

					} else {
						if (fileDescriptor[1] >= 0) {
							if (close(fileDescriptor[1]) < 0) {
								warn("%s", filename);
								return 500;
							}
							fileDescriptor[1] = -1;
						}
						badFile[1] = true;
						fileComplete[1] = true;
						correctFileIndex = 0;
					}
				} else {
					if (comp2and3) {
						if (fileDescriptor[2] >= 0) {
							if (close(fileDescriptor[2]) < 0) {
								warn("%s", filename);
								return 500;
							}
							fileDescriptor[2] = -1;
						}
						badFile[2] = true;
						fileComplete[2] = true;
						correctFileIndex = 0;
					}
				}
			} else if (badFile[0]) {
				if (comp2and3) {
					for (int i = 0; i < 3; i++) {
						if (close(fileDescriptor[i]) < 0) {
							warn("%s", filename);
						}
						return 500;
					}
				}
			} else if (badFile[1]) {
				if (comp1and3) {
					for (int i = 0; i < 3; i++) {
						if (close(fileDescriptor[i]) < 0) {
							warn("%s", filename);
						}
						return 500;
					}
				}
			} else {
				if (comp1and2) {
					for (int i = 0; i < 3; i++) {
						if (close(fileDescriptor[i]) < 0) {
							warn("%s", filename);
						}
						return 500;
					}
				}
			}
		}


		int fdCheck = 0;
		for (int i = 0; i < fileCount; i++) {
			if (fileDescriptor[i] < 0) {
				fdCheck++;
			}
		}

		if (fdCheck >= fileCount) {
			break;
		}
	}

	contentLength = bytesRead[correctFileIndex];

	pthread_mutex_unlock(mutx);	//end critical region
	
	return 200;
}

int HTTPParse::GetContentLength() {
	return contentLength;
}

char* HTTPParse::GetFilename() {
	return filename;
}

bool HTTPParse::IsValidName(char* filename) {
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

int HTTPParse::HandleBackups(char* filename) {
	contentLength = 0;
	time_t seconds;
	seconds = time(NULL);
	// set folder name
	std::string folderName = "backup-" + std::to_string(seconds);
	std::cout << "[HTTPParse] folderName: " << folderName << '\n';
	// create directory
	const char * directory = folderName.c_str();
	int checkCreatedDirectory = mkdir(directory,0777); 
	std::cout << "[HTTPParse] created directory " << directory << '\n';
	// for each file in the current directory
		// set path name
		// read and write to path name

	struct dirent *directoryPointer;  // Pointer for directory entry 
  
	// opendir() returns a pointer of DIR type.  
    DIR *openedSuccessfully = opendir("."); 
  
    if (openedSuccessfully == NULL) { 
		std::cout << "[HTTPParse] couldn't open directory\n";
        return 500; 
	} 

    while ((directoryPointer = readdir(openedSuccessfully)) != NULL) {
		std::string file = directoryPointer->d_name;
		const char * fileNameStr = file.c_str();
		std::cout << "IsProgramFile" << fileNameStr << IsProgramFile(fileNameStr) << '\n';
		if (!IsProgramFile(fileNameStr)) {
			std::string file = directoryPointer->d_name;
			std::cout << "[HTTPParse] file: " << file << '\n';
			std::string pathnamestr = folderName + "/" + file;
			std::cout << "[HTTPParse] pathName: " << pathnamestr << '\n';
			const char * pathName = pathnamestr.c_str();

			char b[15872];
            memset(b, 0, sizeof b);
			// read from file 
			// open the file
			int fd = open(fileNameStr, O_RDONLY);
			if (fd < 0) {
				if (errno == EACCES) {
					warn("403 %s", fileNameStr);
					// break;
				} else {
					warn("404 %s", fileNameStr);
					// break;
				}
			} else {
				int backupFd = open(pathName, O_CREAT | O_WRONLY | O_TRUNC, S_IRWXU);
				if (backupFd < 0) {
					warn("error opening backup file %s", pathName);
				} else {
					int fileBytesRead = 1;
					while (fileBytesRead != 0) {
						// check if 403 and if its not then continue w this stuff
						fileBytesRead = read(fd, b, sizeof b);
						if (fileBytesRead < 0){
							warn("%s", filename);
							close(fd);
							break;
						}
						// close(fd);
						std::cout << "fileBytesRead" << fileBytesRead << '\n';
						std::cout << "buffer" << b << '\n';
						// write to pathname
						int writtenSuccessfully = write(backupFd, b, fileBytesRead);
						if (writtenSuccessfully < 0) {
							warn("%s", "write()");
							break;
						}
					}
				}
				close(fd);
				close(backupFd);

			}

		}
	}
    closedir(openedSuccessfully);
	return 200;
}

bool HTTPParse::IsProgramFile(const char * f) {
	for (int i = 0; i < 27; i++) {
		if (strcmp(ignore[i], f) == 0) {
			return true;
		}
	}
	return false;
}
