Mitchell Hogle, Gabriella Millican
mhogle, gmillica
Fall 2020
Assignment 2: multi-threaded HTTP server with redundancy

-----------
DESCRIPTION

A program that acts as a basic multithreaded HTTP server that accepts GET and PUT commands
-----------
FILES

main.cpp

servermanager.h

servermanager.cpp

serverconnection.h

serverconnection.cpp

servertools.h

servertools.cpp

globalserverinfo.h

globalserverinfo.cpp

httpparse.h

httpparse.cpp

Makefile

DESIGN.pdf

-----------
INSTRUCTIONS

Run make to compile

Run the server with up to 5 arguments: 
1 and 2: The internet address and port of the server
(ex. ./httpserver localhost 8080)
If the port is left out the server will default to port 80

3 and 4: the -N flag and the number of threads
This argument will specify the number of threadsthe server uses. If these arguments are omitted, the default number of threads used will be 4

5: the Redundancy flag (-r)
If this argument is given, the server will duplicate a PUT file into 3 subdirectories. GETting will result in the server comparing the contents of each copy of the file. If all files are the same, return the contents of one. If 2 files are the same, return the contents of one of the ones that are the same. If none of the files match eachother, return 500.

Once the server is running, anyone can send PUT and GET HTTP requests to the server by using something like curl and specifying the same internet address and port. You can make backups by giving /b as an http filename in a GET request. You can then restore the most recent backup by sending a GET request with the filename /r. If you want to restore a specific backup at a timestamp, do the same thing except make the filename r/[timestamp]. If you want to receive a list of all the backups on the server, send a GET request with the filename /l.
