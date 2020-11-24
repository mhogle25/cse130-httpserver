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

		int newlineCount = 0;
		char* header = new char[1];
		header[0] = '\0';
		char buffer[1];
		while (1) {
			int n = 0;
			std::cout << "Before recv: " << n << '\n'; 
			n = read(serverConnectionData->comm_fd, buffer, 1);
			std::cout << "After recv value: " << n << '\n';

			if (n < 0) {
				warn("recv()");
				break;
			}

			if (n == 0)
				break;
			
			char character[2];
			character[0] = buffer[0];
			character[1] = '\0';
			if (strlen(character) == 1)
				strncat(header, character, strlen(header));
			//std::cout << character;
			/*
			if (strcmp(character, "\r") == 0) {
				if (newlineCount == 0 || newlineCount == 2) {
					newlineCount++;
				} else {
					newlineCount = 0;
				}
			}

			if (strcmp(character, "\n") == 0) {
				if (newlineCount == 3) {
					std::cout << "Found the newline\n";
					break;
				} else if (newlineCount == 1) {
					newlineCount++;
				} else {
					newlineCount = 0;
				}
			}
			*/
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
			char* body = new char[2];
			body[1] = '\0';
			int index = 0;
			while (1) {
				if (cl != -1) {
					if (index >= cl) {
						break;
					}
				}

				int n = recv(serverConnectionData->comm_fd, buffer, 1, 0);

				if (n < 0) {
					char* message = GenerateMessage(500, 0);
					send (serverConnectionData->comm_fd, message, strlen(message), 0);
					break;
				}

				if (n <= 0)
					break;
				
				char character[2];
				character[0] = buffer[0];
				character[1] = '\0';
				if (strlen(character) == 1)
					strncat(body, character, strlen(body));

				std::cout << character;

				index++;
			}
			//parse & handle the body
			int msg = parser->ParseRequestBody(body);
			//send the response
			char* message = GenerateMessage(msg, 0);
			send(serverConnectionData->comm_fd, message, strlen(message), 0);
		}

		delete parser;

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

/*

		std::cout << "[ServerConnection] serverConnectionData->comm_fd: " << serverConnectionData->comm_fd << '\n';
		std::cout << "[ServerConnection] serverConnectionData->index: " << serverConnectionData->index << '\n';

		char* content;

		char buffer[1];
		content = new char[1];
		content[0] = '\0';
		while (recv(serverConnectionData->comm_fd, buffer, 1, 0) > 0) {
			content = strcat(content, buffer);
			std::cout << buffer << '\n';
		}

		int index = 0;
		char* word = new char[1];
		word[0] = '\0';

		int newlineCount = 0;
		for (; index < strlen(content); index++) {
			char character[2];
			character[0] = content[index];
			character[1] = '\0';
			
			word = strcat(word, character);
			
			if (content[index] == '\r') {
				continue;
			}

			if (content[index] == '\n') {
				if (newlineCount < 1) {
					continue;
					newlineCount++;
				} else {
					index++;
					break;
				}


			}

			newlineCount = 0;
		}

		char* head = word;
		word = new char[1];
		word[0] = '\0';

		for (; index < strlen(content); index++) {
			char character[2];
			character[0] = content[index];
			character[1] = '\0';
			
			word = strcat(word, character);
		}

		char* body = word;

		delete[] content;

		HTTPParse* parser = new HTTPParse();

		bool next = false;
		while(1) {
			if (next) {
				content = body;
			} else {
				content = head;
				next = true;
			}
			int message;
			if (parser->GetRequestType() == 1) {
				message = parser->ParseRequestBody(content);
				
				char* msg = GenerateMessage(message, 0);
				send(serverConnectionData->comm_fd, msg, strlen(msg), 0);
				delete parser;
				parser = new HTTPParse();
				break;
			} else {
				message = parser->ParseRequestHeader(content);
				
				if (message != 0) {
					char* msg = GenerateMessage(message, parser->GetContentLength());
					if (message == 200) {
						send(serverConnectionData->comm_fd, msg, strlen(msg), 0);
						for (int i = 0; i < parser->GetContentLength(); i++) {
							char b[1];
							b[0] = parser->body[i];
							send(serverConnectionData->comm_fd, b, 1, 0);
						}

					} else {
						send(serverConnectionData->comm_fd, msg, strlen(msg), 0);
					}
					delete parser;
					parser = new HTTPParse();
					break;
				}
			}
			
		}

		delete[] head;
		delete[] body;
		
		if (parser != NULL) {
			delete parser;
		}

*/