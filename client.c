// The client
#include <stdio.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <string.h>

#define PORT 8080

void connectToServer(int *sock, struct sockaddr_in *address, struct sockaddr_in *serv_addr, char serverIP[]) {
	// Create the socket	
	if ((*sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		fprintf(stderr, "Socket error.");
		exit(EXIT_FAILURE);
	}

	// Zero out the server address memory (get rid of garbage)
	memset(serv_addr, '0', sizeof(*serv_addr));

	serv_addr->sin_family = AF_INET;
	serv_addr->sin_port = htons(PORT);

	// Convert between IPv4 and IPv6 to binary
	// currently uses the localhost address
	if (inet_pton(AF_INET, serverIP, &serv_addr->sin_addr)<=0) { 
		fprintf(stderr, "Invalid address/Address not supported.\n");
		exit(EXIT_FAILURE);
	}

	// Connect
	if (connect(*sock, (struct sockaddr *) serv_addr, sizeof(*serv_addr)) < 0) {
		fprintf(stderr, "Connection failure.\n");
		exit(EXIT_FAILURE);
	}
	
}

/////////////////////////////////
// Main
/////////////////////////////////
int main (int argc, char const *argv[]) {
	struct sockaddr_in address;
	int sock = 0, valread;
	struct sockaddr_in serv_addr;
	char *hello = "Hello from the client.\n";
	char buffer[1024] = {0};

	if (argc < 2) {
		printf("No IP address supplied. Defaulting to localhost.\n");
		connectToServer(&sock, &address, &serv_addr, "127.0.0.1");
	} else {
		// Connect to the specified server (Must supply ip address)
		connectToServer(&sock, &address, &serv_addr, argv[1]);
	}

	// Send a message to the server through the socket
	send(sock, hello, strlen(hello), 0);
	printf("Sent hello to server.\n");

	int exit = 0;

	char msg[1024] = {0};

	while (exit == 0) {

		fgets(msg, 1024, stdin);
		
		send(sock, msg, strlen(msg), 0);

		if (strncmp(msg, "/disconnect", 11) == 0) {
			exit = 1;
		}

		memset(msg, 0, sizeof(msg));
	}
	
	close(sock);

	return 0;
}

