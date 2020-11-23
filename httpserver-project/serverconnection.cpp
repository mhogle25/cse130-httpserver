#include "serverconnection.h"

void ServerConnection::Init(std::queue<ServerConnection*> *q, pthread_mutex_t *m, ServerConnectionData *d) {
	availableServerConnections = q;
	standbyMutex = m;
	serverConnectionData = d;
}

void ServerConnection::SetupConnection() {
	pthread_mutex_unlock(standbyMutex);
}

void ServerConnection::doStuff() {
	while (1) {
		pthread_mutex_lock(standbyMutex);

		std::cout << "[ServerConnection] serverConnectionData->comm_fd: " << serverConnectionData->comm_fd << '\n';
		std::cout << "[ServerConnection] serverConnectionData->index: " << serverConnectionData->index << '\n';

		char buf[SIZE];
		HTTPParse* parser = new HTTPParse();
		while(1) {
			int n = recv(serverConnectionData->comm_fd, buf, SIZE, 0);
			if (n < 0) warn("recv()");
			if (n <= 0) break;
			
			int message;
			if (parser->GetRequestType() == 1) {
				message = parser->ParseRequestBody(buf);
				
				memset(buf, 0, sizeof(buf));	//Clear Buffer
				char* msg = GenerateMessage(message, 0);
				send(serverConnectionData->comm_fd, msg, strlen(msg), 0);
				delete parser;
				parser = new HTTPParse();
			} else {
				message = parser->ParseRequestHeader(buf);
				
				memset(buf, 0, sizeof(buf));	//Clear Buffer
				if (message != 0) {
					char* msg = GenerateMessage(message, parser->GetContentLength());
					if (message == 200) {
						send(serverConnectionData->comm_fd, msg, strlen(msg), 0);
						send(serverConnectionData->comm_fd, parser->body, strlen(parser->body), 0);
					} else {
						send(serverConnectionData->comm_fd, msg, strlen(msg), 0);
					}
					delete parser;
					parser = new HTTPParse();
				}
			}
			
		}
		
		if (parser != NULL) {
			delete parser;
		}

		pthread_mutex_unlock(standbyMutex);
		pthread_mutex_lock(standbyMutex);
		std::cout << "[ServerConnection] Size of Available ServerConnections before push: " << availableServerConnections->size() << '\n';
		availableServerConnections->push(this);
		std::cout << "[ServerConnection] Size of Available ServerConnections after push: " << availableServerConnections->size() << '\n';
		
		close(serverConnectionData->comm_fd);
	}
}

char* ServerConnection::GenerateMessage(int message, int contentLength) {
	if (message == 0) {
		return NULL;
	} else if (message == 200) {
		char clString[SIZE]; 
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
	ServerConnectionData* scd = (ServerConnectionData*)arg;
	scd->thisSC->doStuff();
	
	return NULL;
}

int ServerConnection::GetIndex() {
	return serverConnectionData->index;
}