#include <stdlib.h>

#include "servermanager.h"

void setValues(int argc, char **argv, int &threadCount, bool &redundancy, char *&address, int &port, int *wasFound)
{
	bool foundThreadCountSpecifier = false;
	for (int i = 1; i < argc; i++)
	{
		if (strcmp(argv[i], "-N") == 0)
		{
			threadCount = atoi(argv[i + 1]);
			wasFound[i] = 1;
			wasFound[i + 1] = 1;
			foundThreadCountSpecifier = true;
			i++;
		}
		else if (strcmp(argv[i], "-r") == 0)
		{
			redundancy = true;
			wasFound[i] = 1;
		}
	}

	if (!foundThreadCountSpecifier) {
		threadCount = 4;
	}

	bool isFirstZero = true;
	for (int i = 1; i < argc; i++)
	{
		if (wasFound[i] == 0 && isFirstZero)
		{
			address = argv[i];
			isFirstZero = false;
		}
		else if(wasFound[i] == 0)
		{
			port = atoi(argv[i]);
			break;
		}
	}
}

int main(int argc, char **argv)
{
	int threadCount;
	bool redundancy = false;
	char *address;
	int port = 80;
	if (argc < 3)
	{
		// error, not enough arguments, return
	}
	if (argc > 6)
	{
		// error, too many arguments, return
	}

	if (argc == 3) {
		int wasFound[] = {1, 0, 0};
		setValues(argc, argv, threadCount, redundancy, address, port, wasFound);
	}

	if (argc == 4)
	{
		int wasFound[] = {1, 0, 0, 0};
		setValues(argc, argv, threadCount, redundancy, address, port, wasFound);
	}
	else if (argc == 5)
	{
		int wasFound[] = {1, 0, 0, 0, 0};
		setValues(argc, argv, threadCount, redundancy, address, port, wasFound);
	}
	else
	{
		int wasFound[] = {1, 0, 0, 0, 0, 0};
		setValues(argc, argv, threadCount, redundancy, address, port, wasFound);
	}

	ServerManager serverManager;
	serverManager.Setup(address, port, threadCount, redundancy);
	return 0;
}
