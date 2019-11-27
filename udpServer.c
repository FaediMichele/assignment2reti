#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

#include "myfunction.h"

#define MAX_BUF_SIZE 1024 

int main(int argc, char *argv[]){
  struct sockaddr_in client_addr; // struct containing client address information
  int i;
  ssize_t byteRecv; // Number of bytes received
  ssize_t byteSent; // Number of bytes to be sent
  socklen_t cli_size;
  char receivedData [MAX_BUF_SIZE]; // Data to be received
  char sendData [MAX_BUF_SIZE]; // Data to be sent
  
  cli_size = sizeof(client_addr);
  
  for(;;){
	byteRecv = recvfrom(0, receivedData, MAX_BUF_SIZE, 0, (struct sockaddr *) &client_addr, &cli_size);
  	if(byteRecv == -1){
  		perror("recvfrom");
  		exit(EXIT_FAILURE);
  	}
  	
  	if(strncmp(receivedData, "exit", byteRecv) == 0){
  		perror("Command to stop server received\n");
  		break;
  	}

  	convertToUpperCase(receivedData, byteRecv);
		byteSent = sendto(0, receivedData, byteRecv, 0, (struct sockaddr *) &client_addr, sizeof(client_addr));
  	
  	if(byteSent != byteRecv){
  		perror("sendto");
  		exit(EXIT_FAILURE);
  	}
  }
  
  return 0;
}
