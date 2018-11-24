////////////////////////////////////////////////////////////////////////////////////////////////////
//
// The Server 
//
// For help with polling: https://www.ibm.com/support/knowledgecenter/ssw_ibm_i_71/rzab6/poll.htm
//
////////////////////////////////////////////////////////////////////////////////////////////////////

#include <stdio.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <string.h>
#include <pthread.h>
#include <poll.h>
#include <sys/ioctl.h>
#include <errno.h>

#define PORT 8080
#define BUFLENGTH 1024

#define MAX_CLIENTS 3

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

	int on = 1;
	// Set socket to non blocking
	int rc = ioctl(*server_fd, FIONBIO, (char *)&on);
  	if (rc < 0)
  	{
  	  perror("ioctl() failed");
  	  close(*server_fd);
  	  exit(-1);
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
			printf("Client:\n");
			inet_ntop(AF_INET, &(itr->address.sin_addr), ip, INET_ADDRSTRLEN);
			printf("\t%s\n", ip);
			itr = itr->next;
		}
	}
	printf("Stopping send thread...\n");

	close(sock);

}

/////////////////////////////////
// Main
/////////////////////////////////
int main(int argc, char const *argv[]) {
	// Setup the variables
	int server_fd, new_socket;
	int rc, nfds = 1, timeout, current_size, compress_array;
	struct sockaddr_in address;
	int opt = 1;
	int addrlen = sizeof(address);
	int end_server = 0, close_conn = 0;
	int i, j;	

	char buffer[BUFLENGTH] = {0};

	struct pollfd fds[200];

	// Setup shared memory between processes
	client_list = mmap(NULL, sizeof(client_list) * MAX_CLIENTS, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
	
	pthread_t writethread;	

	// Initialize
	initializeServer(&server_fd, &address, &opt);

	// Listen
	if (listen(server_fd, 3) < 0) {
		fprintf(stderr, "Listen failure.");
		exit(EXIT_FAILURE);
	}

	// Initialize fds space
	memset(fds, 0, sizeof(fds));

	// Setup listening socket
	fds[0].fd = server_fd;
	fds[0].events = POLLIN;	

	// 3 minute timeout
	timeout = (3 * 60 * 1000);

	printf("Server started and listening...\n");

	do {
		// Poll
		rc = poll(fds, nfds, timeout);
		
		if (rc < 0) {
			// Error
			perror("poll() failed\n");
			break;
		}

		if (rc == 0) {
			printf("Timed out.\n");
			break;
		}

		// One or more fds are available if we've reached this
		current_size = nfds;
		for (i = 0; i < current_size; i++) {
			if (fds[i].revents == 0) {
				continue;
			}

			if (fds[i].revents != POLLIN) {
				printf("Error: revents = %d\n", fds[i].revents);
				end_server = 1;
				break;
			}
			
			if (fds[i].fd == server_fd) {
				// Listening socket is readable
				// Accept all incoming connections
				do {
					if ((new_socket = accept(server_fd, NULL, NULL)) < 0) {
						if (errno != EWOULDBLOCK) {
							fprintf(stderr, "Accept failure.\n");
							end_server = 1;
						}
						break;
					}
	
					printf("New connection: %d\n", new_socket);
					fds[nfds].fd = new_socket;
					fds[nfds].events = POLLIN;
					nfds += 1;
				} while (new_socket != -1);
			} else {
				// This isn't the listening socket, so and existing connection must be readable
				close_conn = 0;				

				do {
					rc = recv(fds[i].fd, buffer, sizeof(buffer), 0);
					if (rc < 0) {
						if (errno != EWOULDBLOCK) {
							perror("recv fail\n");
							close_conn = 1;
						}
						break;
					}

					if (rc == 0) {
						printf("Connection closed.\n");
						close_conn = 1;
						break;
					}
		
					buffer[rc] = '\0';
					printf("Recieved from %d: %s", fds[i].fd, buffer);	
					rc = send(fds[i].fd, buffer, rc, 0);
					if (rc < 0) {
						perror("send error\n");
						close_conn = 1;
						break;
					}
				} while (1);

				if (close_conn) {
					close(fds[i].fd);
					fds[i].fd = -1;
					compress_array = 1;
				}
			}
		}

		// remove disconnected fds
		if (compress_array)
    		{
      			compress_array = 0;
      			for (i = 0; i < nfds; i++)
      			{
        			if (fds[i].fd == -1)
        			{
          				for(j = i; j < nfds; j++)
          				{
          				  fds[j].fd = fds[j+1].fd;
          				}
          				i--;
          				nfds--;
        			}
      			}	
		}

	} while (end_server == 0);

	printf("\n\nClosing server..\n\n");

	// close all open sockets
	for (i = 0; i < nfds; i++)
  	{
  		if(fds[i].fd >= 0)
  			close(fds[i].fd);
  	}	

	return 0;
}

