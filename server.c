// The server 
#include <stdio.h>
#include <unistd.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <string.h>
#include <pthread.h>

#define PORT 8080
#define BUFLENGTH 1024

struct client_data {
	int sock;
	struct sockaddr_in address;
};

/////////////////////////////////
// Initialize Server
/////////////////////////////////
void initializeServer(int *server_fd, struct sockaddr_in *address, int *opt) {
	// Create the file descriptor for the socket
	if ((*server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
		fprintf(stderr, "Socket failed.");
		exit(EXIT_FAILURE);
	}

	// Attach the new socket to the specified port
	if (setsockopt(*server_fd, SOL_SOCKET, SO_REUSEADDR, opt, sizeof(*opt))) {
		fprintf(stderr, "Socket option set failure.");
		exit(EXIT_FAILURE);
	}
	address->sin_family = AF_INET;
	address->sin_addr.s_addr = INADDR_ANY;
	address->sin_port = htons(PORT);

	// Bind the socket to the port
	if (bind(*server_fd, (struct sockaddr *)address, sizeof(*address)) > 0) {
		fprintf(stderr, "Bind failure.");
		exit(EXIT_FAILURE);
	}
}

void stopServer() {
	printf("Stopping server...\n");
	exit(EXIT_SUCCESS);
}

void *handle_write(void *sock) {
	char msg[BUFLENGTH] = {0};
	int exit = 0;
	while (exit == 0) {
		fgets(msg, sizeof(msg), stdin);
		send (sock, msg, strlen(msg), 0);
	
		if (strncmp(msg, "/stop", 5) == 0) {
			exit = 1;
		}
	}
	printf("Stopping send thread...\n");

	close(sock);
	stopServer();
}

void *handle_read(void *data) {
	struct client_data *ci = (struct client_data *)data;
	char buffer[BUFLENGTH] = {0};
	int valread;
	char ip[INET_ADDRSTRLEN];
	inet_ntop(AF_INET, &(ci->address.sin_addr), ip, INET_ADDRSTRLEN);
		
	while ((valread = read(ci->sock, buffer, 1024)) > 0) {
		// Read
		buffer[valread] = '\0';
		printf("%s: %s", ip, buffer);
	}

	// Client disconnected
	printf("%s has disconnected.\nStopping read thread...\n", ip);	

	close(ci->sock);
	stopServer();
}

/////////////////////////////////
// Main
/////////////////////////////////
int main(int argc, char const *argv[]) {
	// Setup the variables
	int server_fd, new_socket;
	struct sockaddr_in address;
	int opt = 1;
	int addrlen = sizeof(address);
	
	pthread_t readthread, writethread;	

	// Initialize
	initializeServer(&server_fd, &address, &opt);

	// Listen
	if (listen(server_fd, 3) < 0) {
		fprintf(stderr, "Listen failure.");
		exit(EXIT_FAILURE);
	}
	printf("Server started and listening...\n");
	
	// Accept new connections and assign to socket
	if ((new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t*)&addrlen)) < 0) {
		fprintf(stderr, "Accept failure.");
		exit(EXIT_FAILURE);
	}


	struct client_data *data;
	data->sock = new_socket;
	data->address = address;

	if (pthread_create(&writethread, NULL, handle_write, (void *)new_socket)) {
	}

	if (pthread_create(&readthread, NULL, handle_read, (void *)data)) {
	}
	
	pthread_join(readthread, NULL);
	pthread_join(writethread, NULL);

	// Close the server socket
	close(server_fd);

	return 0;
}

