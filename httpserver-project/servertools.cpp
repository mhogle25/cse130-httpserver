#include"servertools.h"
#include <iostream>

void ServerTools::AppendChar(char*& string, char character) {
	int strLength = strlen(string);
	char* temp = string;
	string = new char[strLength + 2];
	for (int i = 0; i < strLength; i++) {
		string[i] = temp[i];
	}
	delete[] temp;
	string[strLength] = character;
	string[strLength + 1] = '\0';
}

char* ServerTools::AppendString(char*& destination, char* suffix, int count) {
	std::cout << "[server tools] inside append string" << std::endl;
	/*int destinationLength = strlen(destination);
    int suffixLength = strlen(suffix);

	char* temp = destination;
    int i = 0;
	destination = new char[destinationLength + suffixLength + 1];
	for (; i < destinationLength; i++) {
		destination[i] = temp[i];
	}
    delete[] temp;

    for (; i < suffixLength; i++) {
		destination[i] = suffix[i];
	}
	destination[destinationLength + suffixLength] = '\0';*/
	char* toReturn = new char[count];
	strncpy(toReturn, destination, strlen(destination));
	strncat(toReturn, suffix, strlen(suffix));

	return toReturn;
}

void ServerTools::GetSubstringFront(char*& destination, char* source, int bytes) {
	if (bytes > strlen(source)) {
		warn("GetSubstringFront()");
		return;
	}

	if (destination != NULL) {
		delete[] destination;
	}

	destination = new char[bytes + 1];
	for (int i = 0; i < bytes; i++) {
		destination[i] = source[i];
	}

	destination[bytes] = '\0';
}

