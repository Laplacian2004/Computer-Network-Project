

CC = gcc
CFLAGS = -lcrypt -Wall -g

SERVER_EXEC = server
CLIENT_EXEC = client

SERVER_SRC = server2.c
CLIENT_SRC = client.c

all: $(SERVER_EXEC) $(CLIENT_EXEC)

$(SERVER_EXEC): $(SERVER_SRC)
	$(CC) $(SERVER_SRC) $(CFLAGS) -o $(SERVER_EXEC) 

$(CLIENT_EXEC): $(CLIENT_SRC)
	$(CC) $(CLIENT_SRC) $(CFLAGS) -o $(CLIENT_EXEC)
trun:
	truncate -s 0 users.txt

clean:
	rm -f $(SERVER_EXEC) $(CLIENT_EXEC)
