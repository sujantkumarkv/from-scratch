/* Include standard input/output library for functions like printf */
#include <stdio.h>
/* Include standard library for general purpose functions like exit, memory allocation */
#include <stdlib.h>
/* Include socket library for socket programming functions like socket, bind, listen, accept */
#include <sys/socket.h>
/* Include internet address family structures like sockaddr_in */
#include <netinet/in.h>
/* Include definitions for internet protocol structures (may not be strictly necessary here but often used with netinet/in.h) */
#include <netinet/ip.h>
/* Include string handling functions like strerror */
#include <string.h>
/* Include error number definitions and the errno variable */
#include <errno.h>
/* Include POSIX operating system API functions like close, setbuf */
#include <unistd.h>

/* Main function - entry point of the program */
int main() {
	/* 
	 * Disable output buffering for stdout and stderr.
	 * This ensures that any output (like printf statements) is immediately visible, 
	 * which is helpful for debugging, especially when the program might crash or exit unexpectedly.
	 * By default, C might buffer output, meaning it waits until a certain amount of data is ready 
	 * or a newline is encountered before actually writing it out.
	 */
	setbuf(stdout, NULL);
 	setbuf(stderr, NULL);

	printf("Logs from your program will appear here!\n");

	/* 
	 * Variable declarations for socket programming:
	 * server_fd: Integer to hold the file descriptor for the server socket. 
	 *            File descriptors are small non-negative integers that the kernel uses 
	 *            to identify open files, sockets, pipes, etc.
	 * client_addr_len: Integer to store the size of the client address structure. 
	 *                  This is needed by the accept() function. 
	 *                  Note: The correct type for this is socklen_t, using int might cause warnings
	 *                  as seen in your build output (`-Wpointer-sign` warning).
	 * client_addr: A structure of type sockaddr_in to hold the address information 
	 *              (IP address, port number) of the connecting client.
	 */
	int server_fd;
	/* Note: The correct type is socklen_t, using int might cause warnings. */
	/* socklen_t client_addr_len; */
	socklen_t client_addr_len; 
	struct sockaddr_in client_addr;
	
	/*
	 * Create a new socket:
	 * AF_INET: Specifies the address family (IPv4 internet protocols).
	 * SOCK_STREAM: Specifies the socket type (TCP - sequenced, reliable, two-way, connection-based byte streams).
	 * 0: Specifies the protocol (IPPROTO_TCP for SOCK_STREAM, usually 0 allows the system to choose the correct protocol).
	 * Returns a file descriptor for the new socket, or -1 on error.
	 */
	server_fd = socket(AF_INET, SOCK_STREAM, 0);
	if (server_fd == -1) {
		/* 
		 * Error handling: If socket() returns -1, an error occurred.
		 * strerror(errno): Returns a string describing the error code stored in the global variable `errno`.
		 * Print an error message and exit the program with a non-zero status code indicating failure.
		 */
		printf("Socket creation failed: %s...\n", strerror(errno));
		return 1; /* Exit with error code 1 */
	}
	
	/*
	 * Allow reusing the local address (port) immediately after the server closes.
	 * This is crucial for development and testing, where the server might be stopped and restarted frequently.
	 * Without this option, the operating system might keep the port in a TIME_WAIT state for a while, 
	 * preventing the server from immediately binding to the same port again, leading to an "Address already in use" error.
	 * SOL_SOCKET: Specifies that the option is at the socket level.
	 * SO_REUSEADDR: The specific option to enable address reuse.
	 * &reuse: Pointer to the integer variable `reuse` (set to 1, meaning 'true' or 'enable').
	 * sizeof(reuse): Size of the option value.
	 */
	int reuse = 1;
	if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse)) < 0) {
		/* Error handling for setsockopt */
		printf("SO_REUSEADDR failed: %s \n", strerror(errno));
		return 1; /* Exit with error code 1 */
	}
	
	/*
	 * Define the server address structure (sockaddr_in):
	 * .sin_family: Address family, must match the socket's family (AF_INET for IPv4).
	 * .sin_port: Port number in network byte order. htons() (Host TO Network Short) converts 
	 *            the port number (4221) from host byte order to network byte order, which is 
	 *            required for network protocols. Different systems might store multi-byte numbers 
	 *            differently (little-endian vs. big-endian), network byte order (big-endian) ensures consistency.
	 * .sin_addr: IP address. 
	 *            .s_addr: The actual IP address field.
	 *            htonl(INADDR_ANY): Converts the special address INADDR_ANY from host byte order to network byte order.
	 *                             INADDR_ANY (usually 0.0.0.0) means the server will accept connections on any 
	 *                             available network interface on the machine (e.g., localhost, Ethernet, Wi-Fi).
	 */
	struct sockaddr_in serv_addr = { .sin_family = AF_INET ,
									 .sin_port = htons(4221),
									 .sin_addr = { htonl(INADDR_ANY) },
									};
	
	/*
	 * Bind the socket to the specified address and port:
	 * Associates the socket (server_fd) with the network address and port number defined in serv_addr.
	 * This step tells the OS that this specific socket should receive incoming traffic destined for 
	 * that IP address and port.
	 * server_fd: The socket file descriptor.
	 * (struct sockaddr *) &serv_addr: A pointer to the server address structure. It's cast to 
	 *                                  a generic sockaddr pointer, as bind() works with different address types.
	 * sizeof(serv_addr): The size of the server address structure.
	 * Returns 0 on success, -1 on error.
	 */
	if (bind(server_fd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) != 0) {
		/* Error handling for bind */
		printf("Bind failed: %s \n", strerror(errno));
		return 1; /* Exit with error code 1 */
	}
	
	/*
	 * Set the maximum number of pending connections that can be queued for the socket.
	 * If multiple clients try to connect while the server is busy handling another connection, 
	 * the OS will queue up to `connection_backlog` connections. Further connection attempts might be rejected.
	 */
	int connection_backlog = 5;
	/*
	 * Mark the socket as a passive socket, i.e., one that will be used to accept incoming connection requests.
	 * server_fd: The socket file descriptor to listen on.
	 * connection_backlog: The maximum length of the queue for pending connections.
	 * Returns 0 on success, -1 on error.
	 */
	if (listen(server_fd, connection_backlog) != 0) {
		/* Error handling for listen */
		printf("Listen failed: %s \n", strerror(errno));
		return 1; /* Exit with error code 1 */
	}
	
	/* Log message indicating the server is ready and waiting for connections. */
	printf("Waiting for a client to connect...\n");
	
	/* 
	 * Set the size of the client address structure. This is required by accept().
	 * It's an "value-result" argument for accept: 
	 * Before the call, it holds the size of the buffer pointed to by client_addr.
	 * After the call, it holds the actual size of the client address returned.
	 */
	client_addr_len = sizeof(client_addr);
	
	/*
	 * Accept an incoming connection on the listening socket:
	 * This function blocks (waits) until a client attempts to connect to the server.
	 * When a connection arrives, it creates a *new* socket for communication with that specific client
	 * and fills in the client_addr structure with the client's address details.
	 * server_fd: The listening socket descriptor.
	 * (struct sockaddr *) &client_addr: Pointer to a sockaddr structure (or sockaddr_in) where the 
	 *                                  connecting client's address information will be stored.
	 * &client_addr_len: Pointer to the variable holding the size of the client_addr structure. 
	 *                   On return, it will contain the actual size of the client address.
	 *                   Note: This should technically be a pointer to socklen_t.
	 * Returns a new socket file descriptor for the accepted connection, or -1 on error.
	 * The original server_fd remains listening for more connections. 
	 * (Although in this simple example, we only accept one connection).
	 * --- This simple version does not store or use the returned client socket descriptor. ---
	 */
	// accept the connection
	int client_fd = accept(server_fd, (struct sockaddr *)&client_addr, &client_addr_len);
	if (client_fd == -1) {
		printf("Accept failed: %s \n", strerror(errno));
		return 1; /* Exit with error code 1 */
	}
	/* Log message indicating a client has successfully connected. */
	printf("Client connected\n");
	
	// parse the request URL and respond accordingly
	char buffer[1024];
	ssize_t bytes_received = recv(client_fd, buffer, sizeof(buffer) - 1, 0);
	if (bytes_received < 0) {
		printf("Receive failed: %s \n", strerror(errno));
		close(client_fd);
		return 1; /* Exit with error code 1 */
	}
	buffer[bytes_received] = '\0';

	// Debug: Print received request
	printf("Received Request:\\n%s\\n", buffer); 

	// Parse the request line (very basic parsing)
	char method[16];
	char path[512]; // Increased path buffer size just in case
	char version[16];
	// Use sscanf to extract parts. Check return value for robustness (should be 3).
	int parsed_items = sscanf(buffer, "%15s %511s %15s", method, path, version);

	char *response;
	// Check if parsing was successful and the path matches "/"
	if (parsed_items == 3 && strcmp(path, "/") == 0) {
		response = "HTTP/1.1 200 OK\r\n\r\n";
	} else {
		// Default to 404 if path is not "/" or parsing failed
		response = "HTTP/1.1 404 Not Found\r\n\r\n";
	}

	// and now return the appropriate HTTP response
	int bytes_sent = send(client_fd, response, strlen(response), 0);
	if (bytes_sent == -1) {
		printf("Send failed: %s \n", strerror(errno));
		return 1; /* Exit with error code 1 */
	}

	// close connection with the specific client
	close(client_fd);
	
	/* 
	 * Close the listening server socket. 
	 * In this simple program, we close it immediately after accepting one connection.
	 * A real server would typically keep the listening socket open and accept more connections, 
	 * possibly in a loop or using threads/processes to handle multiple clients concurrently.
	 * The connection established by accept() would also need to be closed eventually 
	 * (using the file descriptor returned by accept, which we ignored here).
	 */
	close(server_fd);

	/* Return 0 to indicate successful program execution. */
	return 0;
}
