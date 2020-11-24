#ifndef SERVER_TOOLS
#define SERVER_TOOLS

#include<stdio.h>
#include<string.h>
#include<unistd.h>
#include<stdlib.h>
#include<err.h>

class ServerTools {
public:
    static void AppendChar(char*&, char);
    static void AppendString(char*&, char*);
    static void GetSubstringFront(char*&, char*, int);
};

#endif