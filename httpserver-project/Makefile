httpserver: main.o servermanager.o httpparse.o globalserverinfo.o serverconnection.o
	g++ main.o servermanager.o httpparse.o globalserverinfo.o serverconnection.o -o httpserver -lpthread

main.o: main.cpp
	g++ -c -pthread main.cpp -g

servermanager.o: servermanager.cpp servermanager.h
	g++ -c -pthread servermanager.cpp -g

httpparse.o: httpparse.cpp httpparse.h
	g++ -c -pthread httpparse.cpp -g

globalserverinfo.o: globalserverinfo.cpp globalserverinfo.h
	g++ -c -pthread globalserverinfo.cpp -g

serverconnection.o: serverconnection.cpp serverconnection.h
	g++ -c -pthread serverconnection.cpp -g

target: dependencies
	action

clean:
	rm *.o httpserver
