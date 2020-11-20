httpserver: main.o servermanager.o httpparse.o globalserverinfo.o
	g++ main.o servermanager.o httpparse.o globalserverinfo.o -o httpserver

main.o: main.cpp
	g++ -c -pthread main.cpp

servermanager.o: servermanager.cpp servermanager.h
	g++ -c -pthread servermanager.cpp

httpparse.o: httpparse.cpp httpparse.h
	g++ -c -pthread httpparse.cpp

globalserverinfo.o: globalserverinfo.cpp globalserverinfo.h
	g++ -c -pthread globalserverinfo.cpp

target: dependencies
	action

clean:
	rm *.o httpserver
