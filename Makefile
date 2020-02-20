CFLAGS=-Wpedantic -Wall -D_REENTRANT

all:	superserver.o

debug: superserver.c
	gcc -DDEBUG ${CFLAGS} superserver.c -o superserver.o

service:	tcpServer.o udpServer.o

client:	tcpClient.o udpClient.o

tcpClient.o: tcpClient.c
	gcc tcpClient.c -o tcpClient.o

udpClient.o: udpClient.c
	gcc udpClient.c -o udpClient.o

superserver.o: superserver.c
	gcc ${SYMBOL} ${CFLAGS} superserver.c -o superserver.o

tcpServer.o: tcpServer.c
	gcc ${LIBFLAGS} tcpServer.c -o tcpServer.o

udpServer.o: udpServer.c
	gcc ${LIBFLAGS} udpServer.c -o udpServer.o

.PHONY: clean cleanClient

clean:
	rm -f superserver.o tcpServer.o udpServer.o

cleanClient:
	rm -f udpClient.o tcpClient.o
