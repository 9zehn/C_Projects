#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include "util.h"

int client_socket = -1;
char username[MAX_NAME_LEN + 1];
char inbuf[BUFLEN + 1];
char outbuf[MAX_MSG_LEN + 1];

int handle_stdin() {
	printf("[%s]: ", username);
	int len = 0;
	int c;
	// Parse stdin
	while ((c = getchar()) != EOF && c != '\n') {
		// Check if the message has gone longer than max length
        	if (len >= MAX_MSG_LEN) {
            		fprintf(stderr, "Sorry, limit your message to 1 line of at most %d characters.\n", MAX_MSG_LEN);
            		// Get the rest of the message out of the way
			while ((c = getchar()) != EOF && c != '\n');
            		return 0;
        	}
        	
		outbuf[len++] = c;
    	}

	// Null terminate it
    	outbuf[len] = '\0';
    	
	// Check if the message is "bye" and close out 
	if (strcmp(outbuf, "bye") == 0) {
       		printf("Goodbye.\n");
       		close(client_socket);
       		exit(EXIT_SUCCESS);
    	}
    	
	// Use strcat to put a \n at the end
	strcat(outbuf, "\n");

	// Send the message
    	if (send(client_socket, outbuf, strlen(outbuf), 0) == -1) {
       		fprintf(stderr, "Failed to send message: %s\n", strerror(errno));
       		close(client_socket);
       		exit(EXIT_FAILURE);
	}
	return 0;
}

int handle_client_socket() {
	int num_bytes;
	if ((num_bytes = recv(client_socket, inbuf, BUFLEN, 0)) == -1) {
		if (errno != EINTR) {
			fprintf(stderr, "Warning: Failed to receive incoming message.\n");
		}
	} else if (num_bytes == 0) {
		fprintf(stderr, "\nConnection to server has been lost.\n");
		close(client_socket);
		return EXIT_FAILURE;
	} else {
		inbuf[num_bytes] = '\0';
		
		if (strcmp(inbuf, "bye\n") == 0) {
			printf("\nServer initiated shutdown.\n");
			close(client_socket);
			return EXIT_SUCCESS;
		}

		printf("%s", inbuf);
	
	}
	return 0;
}

int main(int argc, char **argv) {
	if (argc != 3) {
		fprintf(stderr, "Usage: %s <server IP> <port>\n", *(argv + 0));
		return EXIT_FAILURE;
	}
	
	// Processing user supplied IP and port number inputs
	char *ip_input = *(argv + 1);
	
	struct sockaddr_in server_addr;
    	memset(&server_addr, 0, sizeof(server_addr));
    	server_addr.sin_family = AF_INET;

	if (inet_pton(AF_INET, ip_input, &server_addr.sin_addr) <= 0) {
		fprintf(stderr, "Error: Invalid <server IP> '%s'. Please input the IP in the correct format.\n", ip_input);
		return EXIT_FAILURE;
	}
	

	char* port_input = *(argv + 2);
	int port;
	if (parse_int(port_input, &port, port_input)) {
		if (port < 1024 || port > 65535) {
			fprintf(stderr, "Error: Invalid <port> '%d'. Please input a port number in the range [1024, 65535].\n", port);
			return EXIT_FAILURE;
		}
	} else {
		fprintf(stderr, "Error: Invalid <port> '%s'. Please input a port number in the range [1024, 65535].\n", port_input);
                return EXIT_FAILURE;
	}

	server_addr.sin_port = htons(port);

	// Setting up the username
	printf("Enter your username (maximum %d characters): ", MAX_NAME_LEN);
	while (1) {
    		if (fgets(username, MAX_NAME_LEN + 1, stdin) != NULL) {
        		int len = strlen(username);
        		if (len == 1) {
            			printf("Retry: ");
            			continue;
        		}
        		if (len > MAX_NAME_LEN) {
            			printf("Sorry, limit your username to %d characters.\n", MAX_NAME_LEN);
				memset(username, 0, sizeof(username));
            			continue;
        		}
       			if (username[len-1] == '\n') {
            			username[len-1] = '\0';
            			break;
        		} else {
            			printf("Sorry, limit your username to %d characters.\n", MAX_NAME_LEN);
            			memset(username, 0, sizeof(username));
        		}
    		
		} else {
        		printf("Failed to read input.\n");
        		exit(EXIT_FAILURE);
    		}
	}
	username[strlen(username)] = '\0';
	printf("Hello, %s. Let's try to connect to the server.\n", username);

	// Setting up and connecting sockets
	if ((client_socket = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		fprintf(stderr, "Error: Issue setting up socket. %s\n", strerror(errno));
		return EXIT_FAILURE;
	}

	if (connect(client_socket, (struct sockaddr *) &server_addr, sizeof(server_addr)) < 0) {
		fprintf(stderr, "Error: Issue connecting the sockets. %s\n", strerror(errno));
		close(client_socket);
		return EXIT_FAILURE;
	}

	// Recieving welcome message
	int num_bytes;
	if ((num_bytes = recv(client_socket, inbuf, MAX_MSG_LEN - 1, 0)) < 0) {
		fprintf(stderr, "Error: Issue recieving welcome message. %s\n", strerror(errno));
		close(client_socket);
		return EXIT_FAILURE;
	} else if (num_bytes == 0) {
		fprintf(stderr, "Error: Unexpected server close. %s\n", strerror(errno));
		close(client_socket);
		return EXIT_FAILURE;
	} else {
		if (inbuf[num_bytes] != '\0') {
			inbuf[num_bytes] = '\0';
		}
	}

	printf("\n%s\n\n\n", inbuf);
	
	// Sending username to server
	if (send(client_socket, username, sizeof(username), 0) < 0) {
		fprintf(stderr, "Error: Issue sending username to chat server. %s\n", strerror(errno));
		close(client_socket);
		return EXIT_FAILURE;
	}


	fd_set sockset;
	int max_socket;
	while (1) {
		FD_ZERO(&sockset);
		FD_SET(STDIN_FILENO, &sockset);
    		FD_SET(client_socket, &sockset);
		int max_socket = (STDIN_FILENO > client_socket) ? STDIN_FILENO : client_socket;

		if (select(max_socket + 1, &sockset, NULL, NULL, NULL) == -1) {
        		if (errno == EINTR) {
            			continue;
			}
			fprintf(stderr, "Error: Issue selecting file descriptors: %s\n", strerror(errno));
        		close(client_socket);
        		exit(EXIT_FAILURE);
    		}

    		if (FD_ISSET(STDIN_FILENO, &sockset)) {
        		if (handle_stdin() == -1) {
            			close(client_socket);
            			exit(EXIT_FAILURE);
        		}
    		}
    		
		if (FD_ISSET(client_socket, &sockset)) {
        		if (handle_client_socket() == -1) {
            			close(client_socket);
           	 		exit(EXIT_FAILURE);
        		}
    		}
	}	

}
