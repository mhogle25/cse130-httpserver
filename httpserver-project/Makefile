httpserver: main.o servermanager.o httpparse.o globalserverinfo.o serverconnection.o
	g++ -g main.o servermanager.o httpparse.o globalserverinfo.o serverconnection.o -o httpserver -lpthread

main.o: main.cpp
	g++ -c -g -pthread main.cpp

servermanager.o: servermanager.cpp servermanager.h
	g++ -c -g -pthread servermanager.cpp

httpparse.o: httpparse.cpp httpparse.h
	g++ -g -c -pthread httpparse.cpp

globalserverinfo.o: globalserverinfo.cpp globalserverinfo.h
	g++ -c -g -pthread globalserverinfo.cpp

serverconnection.o: serverconnection.cpp serverconnection.h
	g++ -c -g -pthread serverconnection.cpp

target: dependencies
	action

clean:
	rm *.o httpserver