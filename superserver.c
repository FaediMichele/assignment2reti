/****************************************************************************
 *
 * Compile with:
 * 	gcc superserver.c -o superserver
 * Or:
 * 	gcc -DDEBUG superserver.c -o superserver
 *
 * Run with:
 * 	superserver
 * Or:
 * 	superserver -v
 *
 ****************************************************************************/
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <netinet/in.h>
#include <signal.h>
#include <errno.h>
#include <unistd.h>
#include <limits.h>
#include "myfunction.h"

#define MAX_SERVICES 10
#define BACK_LOG 10

#define WAIT 0
#define NOWAIT 1

//Service structure definition goes here
typedef struct 
{
	int transport_protocol;
	int wait_type;
	int service_port;
	char service_path[4096];
	char service_name[255];
	int socket_file_descriptor;
	int PID; /* only for WAIT */
} service_t;

//Constants and global variable declaration goes here
service_t services[MAX_SERVICES];
int servicesFound = 0;
int maxSFD = -1;

int verbose = 0;

fd_set set;

//Function prototype devoted to handle the death of the son process
void handle_signal (int sig);


// Covert a line in a service
// Must have this structure:
// 	nameProgram nameService tcp/udp port wait/nowait
int parseLine(service_t *service, char* line){
	fflush(stdout);
	char* paramether = strtok(line, " ");

	// path
	if(strlen(paramether) >= sizeof(service->service_path)) { return 0; }
	strcpy(service->service_path, paramether);

	// name 
	paramether = strtok(NULL, " ");
	if(strlen(paramether) >= sizeof(service->service_name)) { return 0; }
	strcpy(service->service_name, paramether);

	// convert the line after the names to uppercase
	line = paramether + countStringLength(paramether) + 1;
	convertToUpperCase(line, countStrLen(line));

	// protocol
	paramether = strtok(NULL, " ");	
	if(strlen(paramether) > 3 && (!strcmp(paramether, "TCP") && !strcmp(paramether, "UDP"))) { return 0; }
	service->transport_protocol = !strcmp(paramether, "TCP")? IPPROTO_TCP : IPPROTO_UDP; // strcmp return 0 if equal

	// port
	paramether = strtok(NULL, " ");
	if(strlen(paramether) > 5 || !atoi(paramether)) { return 0; }
	service->service_port = atoi(paramether);
	if(service->service_port > 65535 && service->service_port <= 0) { return 0; }

	// wait
	paramether = strtok(NULL, "\n");
	if(strlen(paramether) > 6) { return 0; }
	if(!strcmp(paramether, "WAIT") && !strcmp(paramether, "NOWAIT")) { return 0; }
	service->wait_type = strcmp(paramether, "NOWAIT") == WAIT;
	return 1;
}

// Print the service found
void printServices(){
	int i;
	for(i=0; i < servicesFound; i++){
		printf("[%s] \t- Wait type = %d\tPort of service = %d \tProtocol = %d\tPID = %d\n", services[i].service_path,
			services[i].wait_type, services[i].service_port, services[i].transport_protocol, services[i].PID);
	}
}


// Load the services from a file
void loadServices(char* nameOfFile){
	if(nameOfFile == NULL){ return; }
	FILE *file = fopen(nameOfFile, "r");
	char line[255];
	if(file == NULL){ printf("error loading file...\n"); return; }
	while ( servicesFound < MAX_SERVICES && fgets(line, sizeof(line), file)) {
		if(!parseLine(&services[servicesFound], line)){
			printf("Error reading line n°%d from '%s'\n", servicesFound, nameOfFile);
			continue;
		}
		services[servicesFound].PID = -1;
		servicesFound++;
	}
	fclose(file);
}

// Create the socket for each service
void createSockets(){
	int i;
	struct sockaddr_in server; // address of the server
	server.sin_family = AF_INET;
	server.sin_addr.s_addr = INADDR_ANY;
	for(i = 0; i < servicesFound; i++){
		int type = services[i].transport_protocol == IPPROTO_TCP? SOCK_STREAM : SOCK_DGRAM;
		server.sin_port = htons(services[i].service_port);
		services[i].socket_file_descriptor = socket(AF_INET, type, services[i].transport_protocol);
		if(services[i].socket_file_descriptor < 0){
			printf("Could not create the socket. ERROR %d\n", services[i].socket_file_descriptor);
		}
		if(services[i].socket_file_descriptor > maxSFD){
			maxSFD = services[i].socket_file_descriptor;
		}
		if(bind(services[i].socket_file_descriptor, (struct sockaddr *) &server, sizeof(server)) < 0){
			printf("Error bindin address %d\t%s\n", i, strerror(errno));
		}
		if(services[i].transport_protocol == IPPROTO_TCP &&
			listen(services[i].socket_file_descriptor, BACK_LOG) < 0){
			printf("Could not listen the socket %d\t%s\n", i, strerror(errno));
		}
	}
}

// Fill the select paramether
void fillSelect(){
	FD_ZERO(&set);
	int i;
	for(i = 0; i < servicesFound; i++) {
		if(services[i].PID < 0) {
			FD_SET(services[i].socket_file_descriptor, &set);
		}
	}
}

// Handle the service. Call the fork
void handleService(service_t *service, char **env){
	struct sockaddr_in client;
	socklen_t size= sizeof(client);
	int newSfd = service->socket_file_descriptor;
	int pid;
	if(service->transport_protocol == IPPROTO_TCP){
		newSfd = accept(service->socket_file_descriptor,
		(struct sockaddr *) &client, &size);
		if(newSfd < 0){
			printf("ERROR - %s", strerror(errno));
			fflush(stdout);
			return;
		}
	}
	pid = fork(); // if 0 is son. else is the pid of the son
	if(pid != 0){
		if(service->transport_protocol == IPPROTO_TCP){
			close(newSfd);
		}
		if(service->wait_type == WAIT){
			service->PID = pid;
			FD_CLR(service->socket_file_descriptor, &set);
		#ifdef DEBUG
			printf("Services in wait %s\n", service->service_name);
		#else
			if(verbose){
				printf("Services in wait %s\n", service->service_name);
			}
		#endif
		}
		return;
	}
	// ONLY CHILD COME HERE

	if(service->transport_protocol == IPPROTO_TCP){
		close(service->socket_file_descriptor);
	}

	fclose(stdin);
	fclose(stdout);
	fclose(stderr);
	dup(newSfd); // with id = 0 stdin
	dup(newSfd); // with id = 1 stdout
	dup(newSfd); // with id = 2 stderr

	if(execle(service->service_path, service->service_name, NULL, env) < 0){
		fprintf(stderr, "ERROR - %s", strerror(errno));
		exit(EXIT_FAILURE);
	}
}

int main(int argc,char **argv,char **env){
	int i; // index
	
	if(argc > 2){
		printf("Usage superserver [-v|--verbose]\n");
		exit(EXIT_FAILURE);
	}
	if(argc > 1){
		if( !strcmp(argv[1], "-v") || !strcmp(argv[1], "--verbose")){
			verbose = 1;
		} else{
			printf("Usage superserver [-v|--verbose]\n");
			exit(EXIT_FAILURE);
		}
	}
	
	printf("SUPER SERVER RUNNING...\n");
	// Server behavior implementation goes here
	loadServices("initd.conf.txt");
#ifdef DEBUG
	printServices();
#else
	if(verbose){
		printServices();
	}
#endif
	createSockets();
	
	signal (SIGCHLD,handle_signal); /* Handle signals sent by son processes - call this function when it's ought to be */
	for(;;){
		fillSelect(&set);

		if((select(maxSFD + 1, &set, NULL, NULL, NULL)) < 0) {
			if(errno == EINTR) {
				continue;
			}
			printf("Error waiting for service - %s\n", strerror(errno));
			continue;
		} else {
			for(i = 0; i < servicesFound; i++){
				if(FD_ISSET(services[i].socket_file_descriptor, &set)){
				#ifdef DEBUG
					printf("Handling service (%s)\n", services[i].service_name);
				#else
					if(verbose){
						printf("Handling service (%s)\n", services[i].service_name);
					}
				#endif
					fflush(stdout);
					handleService(&services[i], env);
				}
			}
		}
		
	}
	printf("SUPER SERVER ENDED\n");
	return 0;
}

// handle_signal implementation
void handle_signal (int sig){
	
	int pid;
	switch (sig) {
		case SIGCHLD :
			// if the process ended was wait add it to the set
			if( (pid = wait(NULL)) < 0){
				perror("wait");
				exit(EXIT_FAILURE);
			}
			for(int i = 0; i < servicesFound; i++){
				if(services[i].PID == pid){
					if(services[i].wait_type == WAIT){
						FD_SET(services[i].socket_file_descriptor, &set);
					#ifdef DEBUG
						printf("Service in handle %s\n", services[i].service_name);
					#else
						if(verbose){
							printf("Service in handle %s\n", services[i].service_name);
						}
					#endif
						services[i].PID = -1;
					} else {
						return;
					}
				}
			}

			break;
		default : printf ("Signal not known!\n");
			break;
	}
}