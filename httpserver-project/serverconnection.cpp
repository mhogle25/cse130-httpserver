#include "serverconnection.h"
#include <string.h>

void ServerConnection::Init(std::queue<ServerConnection*> *q, pthread_mutex_t *m, ServerConnectionData* d) {
	availableServerConnections = q;
	standbyMutex = m;
	serverConnectionData = d;
}

void ServerConnection::SetupConnection() {
	pthread_mutex_unlock(standbyMutex);
}

void ServerConnection::BeginRecv() {
	while (1) {
		pthread_mutex_lock(standbyMutex);

		int newlineCount = 0;
		char* header = new char[1];
		header[0] = '\0';
		char buffer[SIZE + 1];
		while (1) {
			int n = 0;
			n = read(serverConnectionData->comm_fd, buffer, 1);

			if (n < 0) {
				warn("recv()");
				break;
			}

			if (n == 0) {
				break;
			}

			ServerTools::AppendChar(header, buffer[0]);

			char character[2];
			character[0] = buffer[0];
			character[1] = '\0';
			if (strcmp(character, "\r") == 0) {
				if (newlineCount == 0 || newlineCount == 2) {
					newlineCount++;
				} else {
					newlineCount = 0;
				}
			} else if (strcmp(character, "\n") == 0) {
				if (newlineCount == 3) {
					std::cout << "Found the newline\n";
					break;
				} else if (newlineCount == 1) {
					newlineCount++;
				} else {
					newlineCount = 0;
				}
			} else {
				newlineCount = 0;
			}
			
		}

		int msg;
		HTTPParse* parser = new HTTPParse();
		msg = parser->ParseRequestHeader(header);

		if (msg != 0) {
			if (msg == 200) {
				//send the response
				char* message = GenerateMessage(msg, parser->GetContentLength());
				send(serverConnectionData->comm_fd, message, strlen(message), 0);
				//send the body
				for (int i = 0; i < parser->GetContentLength(); i++) {
					char b[1];
					b[0] = parser->body[i];
					send(serverConnectionData->comm_fd, b, 1, 0);
				}

			} else {
				//send the error
				char* message = GenerateMessage(msg, 0);
				send(serverConnectionData->comm_fd, message, strlen(message), 0);
			}
		} else {
			//recv the body
			int cl = parser->GetContentLength();
			char* body = new char[1];
			body[0] = '\0';
			int index = 0;
			int counter = 0;
			std::string content;
			while (1) {
				memset(buffer, 0, sizeof buffer);

				int n = recv(serverConnectionData->comm_fd, buffer, SIZE, 0);
				index++;
				counter +=n;

				if (n < 0) {
					char* message = GenerateMessage(500, 0);
					send (serverConnectionData->comm_fd, message, strlen(message), 0);
					break;
				}

				if (n == 0)
					break;


				char* fullBody = new char[counter + 1];
			        strcpy(fullBody, ServerTools::AppendString(body, buffer, counter));
				fullBody[counter] = '\0';
			        std::string dest(fullBody);
				content += dest;

				if (counter >= cl) {
                	break;
                }
			}
			char *bodyToSend = new char[content.size() -1];
			strcpy(bodyToSend, content.c_str());
			//parse & handle the body
			int msg = parser->ParseRequestBody(bodyToSend);
			//send the response
			char* message = GenerateMessage(msg, 0);
			send(serverConnectionData->comm_fd, message, strlen(message), 0);
		}

		delete parser;

		std::cout << "[ServerConnection] IsConnectionClosed: " << '\n';
		if (shutdown(serverConnectionData->comm_fd, SHUT_RDWR) < 0) {
			warn("shutdown()");
		}
		if (close(serverConnectionData->comm_fd) < 0) {
			warn("close()");
		}
		std::cout << "[ServerConnection] IsConnectionClosed: " << '\n';

		pthread_mutex_unlock(standbyMutex);

		pthread_mutex_lock(standbyMutex);
		std::cout << "[ServerConnection] Size of Available ServerConnections before push: " << availableServerConnections->size() << '\n';
		availableServerConnections->push(this);
		std::cout << "[ServerConnection] Size of Available ServerConnections after push: " << availableServerConnections->size() << '\n';
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
	scd->thisSC->BeginRecv();
	
	return NULL;
}

int ServerConnection::GetIndex() {
	return serverConnectionData->index;
}
