CCFLAGS=-O2

all: server client

server: server.c
	$(CC) $(CCFLAGS) -o server server.c

client: client.c
	$(CC) $(CCFLAGS) -o client client.c

clean:
	$(RM) client server
