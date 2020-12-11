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

