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

/*char* ServerTools::AppendString(char*& destination, char* suffix, int count) {
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
}*/

