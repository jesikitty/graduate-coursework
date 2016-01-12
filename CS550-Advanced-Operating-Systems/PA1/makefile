all: server client
server: server.c
	gcc -o server server.c -Wall
client: client.c
	gcc -pthread -o client client.c -Wall
clean:
	rm client server
