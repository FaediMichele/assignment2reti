CFLAGS=-Wpedantic -Wall -D_REENTRANT

all:	superserver.o tcpServer.o udpServer.o

debug: superserver.c
	gcc -DDEBUG ${CFLAGS} superserver.c -o superserver.o

superserver.o: superserver.c
	gcc ${SYMBOL} ${CFLAGS} superserver.c -o superserver.o

tcpServer.o: tcpServer.c
	gcc ${LIBFLAGS} tcpServer.c -o tcpServer.o

udpServer.o: udpServer.c
	gcc ${LIBFLAGS} udpServer.c -o udpServer.o

.PHONY: clean

clean:
	rm -f superserver tcpServer.o udpServer.o

