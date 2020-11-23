#include "serverconnection.h"
#include <unistd.h>
#include <sys/types.h>
#include <string.h>

void ServerConnection::Init(queue<ServerConnection>* q) {
	availableServerConnections = q;
}

void ServerConnection::SetupConnection(int fd) {
	pthread_t thread[SIZE];
	int counterVal = GlobalServerInfo::GetCounter();
	struct argStruct arguments;
	arguments.connection_fd = fd;
	arguments.thisSc = this;
	pthread_create(&thread[counterVal], NULL, &toProcess, &arguments);
	// availableServerConnections->push(*this);
}

void ServerConnection::handleRequests(int fileDesc) {
	// std::cout << "comm_fd: " << ServerConnection::comm_fd << std::endl;
	// std::cout << "fileDesc: " << fileDesc << std::endl;
	char buf[SIZE];
	HTTPParse* parser = new HTTPParse();
	while(1) {
		int n = recv(fileDesc, buf, SIZE, 0);
		if (n < 0) warn("recv()");
		if (n <= 0) break;
		
		//printf("%s", buf);
		int message;
		if (parser->GetRequestType() == 1) { // PUT
			std::cout << "before getting msg Code:" << fileDesc << std::endl;
			message = parser->ParseRequestBody(buf, fileDesc);
			memset(buf, 0, sizeof(buf));	//Clear Buffer
			char* msg = GenerateMessage(message, 0);
			//printf("%s\n", msg);
			std::cout << "message code" << fileDesc << std::endl;
			send(fileDesc, msg, strlen(msg), 0);
			std::cout << "sent to: " << fileDesc << std::endl;
			delete parser;
			parser = new HTTPParse();
		} else {
			message = parser->ParseRequestHeader(buf, fileDesc);
			
			memset(buf, 0, sizeof(buf));	//Clear Buffer

			if (message == 1) {
				// we're done
			} else if (message != 0) { // for errors
				char* msg = GenerateMessage(message, parser->GetContentLength());
				if (message == 200) {
					//printf("%s", msg);
					send(fileDesc, msg, strlen(msg), 0);
					//printf("%s\n", parser->body);
					send(fileDesc, parser->body, strlen(parser->body), 0);
				} else {
					//printf("%s", msg);
					send(fileDesc, msg, strlen(msg), 0);
				}
				delete parser;
				parser = new HTTPParse();
			}
		}
	}
	
	if (parser != NULL) {
		delete parser;
	}
	
	close(fileDesc);
	availableServerConnections->push(*this);
}

char* ServerConnection::GenerateMessage(int message, int contentLength) {
	if (message == 0) {
		return NULL;
	} else if (message == 200) {
		char clString[SIZE];
		//printf("%i", contentLength);
		sprintf(clString, "%i", contentLength);
		char* msg = strdup("HTTP/1.1 200 OK\r\nContent-Length: ");
		msg = strcat(msg, clString);
		msg = strcat(msg, "\r\n\r\n");
		return msg;
	} else if (message == 201) {
		return strdup("HTTP/1.1 201 OK\r\nContent-Length: 0\r\n\r\n");
	} else if (message == 400) {
		return strdup("HTTP/1.1 400 Bad Request\r\nContent-Length: 0\r\n\r\n");
	} else if (message == 403) {
		return strdup("HTTP/1.1 403 Forbidden\r\nContent-Length: 0\r\n\r\n");
	} else if (message == 404) {
		return strdup("HTTP/1.1 404 Not Found\r\nContent-Length: 0\r\n\r\n");
	} else if (message == 500) {
		return strdup("HTTP/1.1 500 Internal Server Error\r\nContent-Length: 0\r\n\r\n");
	} else {
		//Something is wrong
		return NULL;
	}
}

void* ServerConnection::toProcess(void* arg) {
	//ServerConnection* scPointer = (ServerConnection*)arg;
	argStruct* test = (argStruct*)arg;
	test->thisSc->handleRequests(test->connection_fd);
}
