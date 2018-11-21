// The server 
#include <stdio.h>
#include <unistd.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <string.h>

#define PORT 8080

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

/////////////////////////////////
// Main
/////////////////////////////////
int main(int argc, char const *argv[]) {
	// Setup the variables
	int server_fd, new_socket, valread;
	struct sockaddr_in address;
	int opt = 1;
	int addrlen = sizeof(address);
	char buffer[1024] = {0};
	
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

	// Read from client socket
	while ((valread = read(new_socket, buffer, 1024)) > 0) {
		// Read
		buffer[valread] = '\0';
		char ip[INET_ADDRSTRLEN];
		inet_ntop(AF_INET, &(address.sin_addr), ip, INET_ADDRSTRLEN);
		printf("%s: %s", ip, buffer);
	//	memset(buffer, 0, sizeof(buffer));
	}
	
	// Send
	//send(new_socket, hello, strlen(hello), 0);
	//printf("Sent a greeting to the new connection.\n");

	// Close the server socket
	close(server_fd);

	return 0;
}

