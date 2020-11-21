#include "serverconnection.h"

void ServerConnection::Init(queue<ServerConnection>* q) {
	availableServerConnections = q;
}

void ServerConnection::SetupConnection(int fd) {
	comm_fd = fd;
	
	
	
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
		delete parser;
	}
	
	close(comm_fd);
	
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


