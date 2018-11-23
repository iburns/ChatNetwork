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
	struct client_data *next;
} *client_list;

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
		close(server_fd);
		exit(EXIT_FAILURE);
	}
}

void stopServer() {
	printf("Stopping server...\n");
	exit(EXIT_SUCCESS);
}

void addClientData(struct client_data *data) {
	if (client_list == NULL) {
		client_list = data;
	} else {
		struct client_data *last = client_list;
		while (last->next != NULL) {
			last = last->next;
		}
		last->next = data;
	}
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

		struct client_data *itr = client_list;
		char ip[INET_ADDRSTRLEN];
		inet_ntop(AF_INET, &(itr->address.sin_addr), ip, INET_ADDRSTRLEN);
		printf("Connected clients: %s\n", ip);
		while (itr->next) {
			printf("test\n");
			inet_ntop(AF_INET, &(itr->address.sin_addr), ip, INET_ADDRSTRLEN);
			printf("Connected clients: %s\n", ip);
			itr = itr->next;
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

	while(1) {	
		// Accept new connections and assign to socket
		if ((new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t*)&addrlen)) < 0) {
			fprintf(stderr, "Accept failure.");
			exit(EXIT_FAILURE);
		}
	
		int pid = fork();

		if (pid == 0) {
			// is the child
			close(server_fd);
			
			struct client_data *data;
			data->sock = new_socket;
			data->address = address;
			data->next = NULL;
			addClientData(data);

			
			if (pthread_create(&writethread, NULL, handle_write, (void *)new_socket)) {
				fprintf(stderr, "Write thread create error.\n");
				exit(EXIT_FAILURE);
			}
			
			
			if (pthread_create(&readthread, NULL, handle_read, (void *)data)) {
				fprintf(stderr, "Read thread create error.\n");
				exit(EXIT_FAILURE);	
			}
			
			pthread_join(readthread, NULL);
			//pthread_join(writethread, NULL);		
			
			exit(EXIT_SUCCESS);
		} else {
			// is server
			close(new_socket);
		}
	}
	return 0;
}

