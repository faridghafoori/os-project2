CC=g++ -w -g
CFLAGS=-c
headers=util.h constants.h
server_objs=server.o util.o 
client_objs=client.o util.o

all: server client

server: $(server_objs) $(headers)
	$(CC) $(server_objs) -o server

client: $(client_objs) $(headers)
	$(CC) $(client_objs) -o client

server.o: server.cpp $(headers)
	$(CC) $(CFLAGS) server.cpp -o server.o

client.o: client.cpp $(headers)
	$(CC) $(CFLAGS) client.cpp -o client.o

util.o: util.cpp $(headers)
	$(CC) $(CFLAGS) util.cpp

clean:
	find . -name "*.o" -type f -delete
	find . -name "*.out" -type f -delete