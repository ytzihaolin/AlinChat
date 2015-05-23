

Server.o:Server.cpp Client.o
	gcc Server.cpp -lpthread -o Server
Client.o:Client.cpp
	gcc Client.cpp -lpthread -o Client
clean:
	rm Server.o Client.o
