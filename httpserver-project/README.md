Mitchell Hogle
mhogle
Fall 2020
Assignment 2: httpserver

-----------
DESCRIPTION

A program that acts as a basic HTTP server that accepts GET and PUT commands
-----------
FILES

main.cpp

servermanager.h

servermanager.cpp

httpparse.h

httpparse.cpp

Makefile

DESIGN.pdf

-----------
INSTRUCTIONS

Run make to compile

Run the server with 2 arguments: the internet address of the server, and the port for it to act on
(ex. ./httpserver localhost 8080)

Once the server is running, anyone can send PUT and GET HTTP requests to the server by using something like curl and specifying the same internet address and port
