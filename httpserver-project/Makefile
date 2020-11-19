httpserver: main.o servermanager.o httpparse.o
	g++ main.o servermanager.o httpparse.o -o httpserver

main.o: main.cpp
	g++ -c main.cpp

servermanager.o: servermanager.cpp servermanager.h
	g++ -c servermanager.cpp

httpparse.o: httpparse.cpp httpparse.h
	g++ -c httpparse.cpp

target: dependencies
	action

clean:
	rm *.o httpserver
