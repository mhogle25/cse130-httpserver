#include "servermanager.h"
#include <stdlib.h>

ServerManager::ServerManager() {
	listen_fd = 0;
	comm_fd = 0;
}

ServerManager::~ServerManager() {
	
}

/*
  getaddr returns the numerical representation of the address
  identified by *name* as required for an IPv4 address represented
  in a struct sockaddr_in.
*/

unsigned long ServerManager::GetAddress(char *name) {
	unsigned long res;
	struct addrinfo hints;
	struct addrinfo* info;

	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_INET;
	if (getaddrinfo(name, NULL, &hints, &info) != 0 || info == NULL) {
		char msg[] = "getaddrinfo(): address identification error\n";
		write(STDERR_FILENO, msg, strlen(msg));
		exit(1);
	}
	res = ((struct sockaddr_in*) info->ai_addr)->sin_addr.s_addr;
	freeaddrinfo(info);
	return res;
}

void ServerManager::Setup(char* address, unsigned short port) {
	struct sockaddr_in servaddr;
 	memset(&servaddr, 0, sizeof servaddr);
	
 	listen_fd = socket(AF_INET, SOCK_STREAM, 0);
 	if (listen_fd < 0) {
 		err(1, "socket()");
 	}
 	
 	servaddr.sin_family = AF_INET;
 	servaddr.sin_addr.s_addr = GetAddress(address);
 	servaddr.sin_port = htons(port);
 	if (bind(listen_fd, (struct sockaddr*) &servaddr, sizeof servaddr) < 0) {
 		err(1, "bind()");
 	}
 
 	if (listen(listen_fd, 500) < 0) {
 		err(1, "listen()");
 	}
 
	while(1) {
		char waiting[] = "waiting for connection\n";
		write(STDOUT_FILENO, waiting, strlen(waiting));
		int comm_fd = accept(listen_fd, NULL, NULL);
		if (comm_fd < 0) {
			warn("accept()");
			continue;
		}
		
		char buf[SIZE];
		HTTPParse* parser = new HTTPParse();
		while(1) {
			int n = recv(comm_fd, buf, SIZE, 0);
			if (n < 0) warn("recv()");
			if (n <= 0) break;
			
			//printf("%s", buf);
			
			int message;
			if (parser->GetRequestType() == 1) {
				message = parser->ParseRequestBody(buf);
				
				memset(buf, 0, sizeof(buf));	//Clear Buffer
				char* msg = GenerateMessage(message, 0);
				//printf("%s\n", msg);
				send(comm_fd, msg, strlen(msg), 0);
				delete parser;
				parser = new HTTPParse();
			} else {
				message = parser->ParseRequestHeader(buf);
				
				memset(buf, 0, sizeof(buf));	//Clear Buffer
				if (message != 0) {
					char* msg = GenerateMessage(message, parser->GetContentLength());
					if (message == 200) {
						//printf("%s", msg);
						send(comm_fd, msg, strlen(msg), 0);
						//printf("%s\n", parser->body);
						send(comm_fd, parser->body, strlen(parser->body), 0);
					} else {
						//printf("%s", msg);
						send(comm_fd, msg, strlen(msg), 0);
					}
					delete parser;
					parser = new HTTPParse();
				}
			}
			

			
		}
		
		if (parser != NULL) {
			err(1, "Connection ended early, could not complete the request");
			delete parser;
		}
		
		close(comm_fd);
	}
}

char* ServerManager::GenerateMessage(int message, int contentLength) {
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
