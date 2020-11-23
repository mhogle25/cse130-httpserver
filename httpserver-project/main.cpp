#include "servermanager.h"

void setValues(int argc, char **argv, int &threadCount, bool &redundancy, char *&address, int &port, int *wasFound)
{
	for (int i = 1; i < argc; i++)
	{
		if (strcmp(argv[i], "-N") == 0)
		{
			threadCount = atoi(argv[i + 1]);
			wasFound[i] = 1;
			wasFound[i + 1] = 1;
			i++;
		}
		else if (strcmp(argv[i], "-r") == 0)
		{
			redundancy = true;
			wasFound[i] = 1;
		}
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
	int threadCount = 4;
	bool redundancy = false;
	char *address;
	int port = 80;
	if (argc < 2)
	{
		// error, not enough arguments, return
	}
	if (argc > 6)
	{
		// error, too many arguments, return
	}

	if (argc == 2) {
		address = argv[1];
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

	char msg1[] = "Address: ";
	write(STDOUT_FILENO, msg1, strlen(msg1));
	write(STDOUT_FILENO, address, strlen(address));
	char msg2[] = "\nPort: ";
	write(STDOUT_FILENO, msg2, strlen(msg2));
	char buffer[32];
	sprintf(buffer, "%d", port);
	write(STDOUT_FILENO, buffer, strlen(buffer));
	memset(buffer, 0, sizeof(buffer));
	char msg3[] = "\nThread Count: ";
	write(STDOUT_FILENO, msg3, strlen(msg3));
	sprintf(buffer, "%d", threadCount);
	write(STDOUT_FILENO, buffer, strlen(buffer));
	char msg4[] = "\nRedundancy: ";
	write(STDOUT_FILENO, msg4, strlen(msg4));
	if (redundancy) {
		char msg5[] = "true\n";
		write(STDOUT_FILENO, msg5, strlen(msg5));
	} else {
		char msg5[] = "false\n";
		write(STDOUT_FILENO, msg5, strlen(msg5));
	}



	ServerManager serverManager;
	serverManager.Setup(address, port, threadCount, redundancy);
	return 0;
}
