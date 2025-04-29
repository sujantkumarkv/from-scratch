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
#include <ctype.h>
// for concurrent connections, using threads
#include <pthread.h>
// For stat() to get file information
#include <sys/stat.h>
// For off_t type used in file size
#include <sys/types.h>
// For PATH_MAX (optional, could use a fixed buffer size)
// #include <limits.h> 

// Global variable to store the directory path provided via command line argument
char *g_directory_path = NULL;

// Function to handle individual client connections in separate threads
void *handle_client(void *client_fd_ptr) {
	// The argument is a pointer to the client file descriptor allocated in main.
	// We dereference it to get the actual file descriptor.
	int client_fd = *(int *)client_fd_ptr;
	// We must free the memory allocated in main for this argument.
	free(client_fd_ptr);

	// Buffer to store incoming request data
	char buffer[1024];
	// Receive data from the client
	ssize_t bytes_received = recv(client_fd, buffer, sizeof(buffer) - 1, 0);
	
	// Error handling for recv
	if (bytes_received < 0) {
		perror("Receive failed"); // Use perror for system call errors
		close(client_fd);
		return NULL; // Exit thread
	}
	// Null-terminate the received data to treat it as a string
	buffer[bytes_received] = '\0';

	// Debug: Print received request
	// printf("Received Request:\n%s\n", buffer); 

	// Variables for parsing the HTTP request line
	char method[16];
	char path[512]; 
	char version[16];
	// Parse the first line (request line)
	int parsed_items = sscanf(buffer, "%15s %511s %15s", method, path, version);

	char *response = NULL;
	size_t response_len = 0;
	// Flag to track if 'response' was dynamically allocated (needs free)
	int use_dynamic_response = 0; 
	// Flag to track if the response was fully sent within a specific route handler
	int response_sent = 0; 

	// --- Start of Request Routing ---
	// Handle root path "/"
	if (parsed_items == 3 && strcmp(path, "/") == 0) {
		response = "HTTP/1.1 200 OK\r\n\r\n";
		response_len = strlen(response);
	} 
	// Handle "/echo/..." paths
	else if (parsed_items == 3 && strncmp(path, "/echo/", 6) == 0) {
		char *echo_str = path + 6; // Point to the string after "/echo/"
		size_t echo_len = strlen(echo_str);

		// Estimate size needed for the response string
		size_t required_size = 100 + echo_len; 
		response = malloc(required_size);
		// Error handling for malloc
		if (response == NULL) {
			perror("Malloc failed for echo response");
			close(client_fd);
			return NULL; // Exit thread
		}
		use_dynamic_response = 1; // Mark response for freeing later
		
		// Safely format the HTTP response string
		int written_len = snprintf(response, required_size,
									"HTTP/1.1 200 OK\r\n"
									"Content-Type: text/plain\r\n"
									"Content-Length: %zu\r\n\r\n"
									"%s",
									echo_len, echo_str);
		// Error handling for snprintf
		if (written_len < 0 || (size_t)written_len >= required_size) {
			fprintf(stderr, "snprintf error or truncation for echo response\n");
			free(response);
			close(client_fd);
			return NULL; // Exit thread
		}
		response_len = (size_t)written_len;
	} 
	// Handle "/user-agent" path
	else if (parsed_items == 3 && strcmp(path, "/user-agent") == 0) {
		char *user_agent = ""; // Default to empty string
		size_t user_agent_len = 0;

		// Find the start of headers (after the first \r\n)
		char *header_start = strstr(buffer, "\r\n");
		if (header_start) {
			header_start += 2; // Move past the first \r\n
			char *current_line = header_start;
			char *buffer_end = buffer + bytes_received; // End of received data

			// Iterate through header lines
			while (current_line < buffer_end && strncmp(current_line, "\r\n", 2) != 0) {
				// Find the end of the current header line
				char *next_line_end = strstr(current_line, "\r\n");
				if (!next_line_end) {
					// Malformed header section
					break; 
				}

				// Check if the line starts with "User-Agent: " (case-insensitive)
				const char *ua_prefix = "User-Agent: ";
				size_t ua_prefix_len = strlen(ua_prefix);
				if (strncasecmp(current_line, ua_prefix, ua_prefix_len) == 0) {
					// Found the User-Agent header
					user_agent = current_line + ua_prefix_len; // Point to the value
					// Calculate the length of the value (up to the \r\n)
					user_agent_len = next_line_end - user_agent; 
					break; // Stop searching once found
				}
				// Move to the start of the next header line
				current_line = next_line_end + 2; 
			}
		}

		// Construct the response
		size_t required_size = 100 + user_agent_len; // Estimate size
		response = malloc(required_size);
		// Error handling for malloc
		if (response == NULL) {
			perror("Malloc failed for user-agent response");
			close(client_fd);
			return NULL; // Exit thread
		}
		use_dynamic_response = 1; // Mark response for freeing

		// Safely format the response string including the User-Agent value
		int written_len = snprintf(response, required_size,
								   "HTTP/1.1 200 OK\r\n"
								   "Content-Type: text/plain\r\n"
								   "Content-Length: %zu\r\n"
								   "\r\n"
								   "%.*s", // Use %.*s to print exactly user_agent_len bytes
								   user_agent_len,      // Value for Content-Length
								   (int)user_agent_len, // Length specifier for %.*s
								   user_agent);         // The User-Agent string itself

		// Error handling for snprintf
		if (written_len < 0 || (size_t)written_len >= required_size) {
			fprintf(stderr, "snprintf error or truncation for user-agent response\n");
			free(response);
			close(client_fd);
			return NULL; // Exit thread
		}
		response_len = (size_t)written_len;
	} 
	// Handle "/files/<filename>" paths
	else if (parsed_items == 3 && strncmp(path, "/files/", 7) == 0) {
		// Check if the directory path was provided via command line
		if (g_directory_path == NULL) {
			fprintf(stderr, "Error: Directory path not specified on startup.\n");
			// 500 because it's a server configuration issue preventing the request
			response = "HTTP/1.1 500 Internal Server Error\r\n\r\n";
			response_len = strlen(response);
			use_dynamic_response = 0;
		} else {
			// Extract the filename from the path (skip "/files/")
			const char *filename = path + 7;
			// Buffer to hold the full path to the requested file
			char full_path[1024]; 
			
			// Construct the full path safely
			int path_len = snprintf(full_path, sizeof(full_path), "%s/%s", g_directory_path, filename);
			
			// Check for path construction errors (e.g., truncation)
			if (path_len < 0 || (size_t)path_len >= sizeof(full_path)) {
				fprintf(stderr, "Error constructing file path (too long?): %s\n", filename);
				// 500 for server-side path issue
				response = "HTTP/1.1 500 Internal Server Error\r\n\r\n"; 
				response_len = strlen(response);
				use_dynamic_response = 0;
			} else {

				// --- Handle GET requests --- 
				if (strcmp(method, "GET") == 0) {
					struct stat file_stat;
					// Use stat() to check if the file exists and get its properties
					if (stat(full_path, &file_stat) == 0) {
						// Check if it's a regular file (not a directory, etc.)
						if (S_ISREG(file_stat.st_mode)) {
							// Get the file size
							off_t file_size = file_stat.st_size;
							// Open the file in binary read mode ("rb")
							FILE *file = fopen(full_path, "rb");
							
							if (file == NULL) {
								// Error opening file (e.g., permissions) - treat as Not Found
								perror("fopen failed for GET");
								response = "HTTP/1.1 404 Not Found\r\n\r\n";
								response_len = strlen(response);
								use_dynamic_response = 0;
							} else {
								// File opened successfully, prepare 200 OK response with file content
								char header_buffer[256]; 
								int header_len = snprintf(header_buffer, sizeof(header_buffer),
														  "HTTP/1.1 200 OK\r\n"
														  "Content-Type: application/octet-stream\r\n"
														  "Content-Length: %lld\r\n\r\n", // %lld for off_t
														  file_size);

								if (header_len < 0 || (size_t)header_len >= sizeof(header_buffer)) {
									fprintf(stderr, "snprintf error or truncation for GET file headers\n");
									fclose(file);
									close(client_fd); 
									return NULL; 
								}

								// Send the headers first
								if (send(client_fd, header_buffer, header_len, 0) != header_len) {
									perror("Failed to send GET file headers completely");
									fclose(file);
									close(client_fd);
									return NULL; 
								}

								// Read file and send content in chunks
								char file_buffer[4096]; 
								size_t bytes_read;
								while ((bytes_read = fread(file_buffer, 1, sizeof(file_buffer), file)) > 0) {
									if (send(client_fd, file_buffer, bytes_read, 0) != (ssize_t)bytes_read) {
										perror("Failed to send file chunk completely (GET)");
										break; // Error or client disconnected
									}
								}

								// Check for file read errors
								if (ferror(file)) {
									perror("fread failed (GET)");
								}

								fclose(file);
								response_sent = 1; // Mark response handled
							}
						} else {
							// Path exists but is not a regular file
							fprintf(stderr, "Access denied: GET '%s' is not a regular file.\n", full_path);
							response = "HTTP/1.1 404 Not Found\r\n\r\n";
							response_len = strlen(response);
							use_dynamic_response = 0;
						}
					} else {
						// stat failed - File likely doesn't exist (errno == ENOENT)
						if (errno != ENOENT) {
							 perror("stat failed for GET file"); 
						}
						response = "HTTP/1.1 404 Not Found\r\n\r\n";
						response_len = strlen(response);
						use_dynamic_response = 0;
					}
				} 
				// --- Handle POST requests --- 
				else if (strcmp(method, "POST") == 0) {
					// --- Find Content-Length Header --- 
					long content_length_val = -1; // Use long for strtol, init to invalid
					char *header_iter = strstr(buffer, "\r\n"); // Find end of request line
					if (header_iter) {
						header_iter += 2; // Point to start of first header
						char *buffer_end = buffer + bytes_received;

						while (header_iter < buffer_end && strncmp(header_iter, "\r\n", 2) != 0) {
							char *next_line_end = strstr(header_iter, "\r\n");
							if (!next_line_end) break; // Malformed headers

							const char *cl_prefix = "Content-Length: ";
							size_t cl_prefix_len = strlen(cl_prefix);
							if (strncasecmp(header_iter, cl_prefix, cl_prefix_len) == 0) {
								char *value_start = header_iter + cl_prefix_len;
								char *endptr;
								errno = 0; // Reset errno before strtol
								content_length_val = strtol(value_start, &endptr, 10);

								// Check strtol errors: range error, no digits, extra chars, negative
								if (errno != 0 || endptr == value_start || (*endptr != '\r' && *endptr != ' ') || content_length_val < 0) {
									fprintf(stderr, "Invalid Content-Length value: %.*s\n", (int)(next_line_end - value_start), value_start);
									content_length_val = -2; // Mark as invalid parse
								}
								break; // Found header (valid or invalid), stop searching
							}
							header_iter = next_line_end + 2;
						}
					}

					if (content_length_val < 0) { // Check if not found (-1) or invalid parse (-2)
						fprintf(stderr, "Missing or invalid Content-Length header for POST /files/\n");
						response = "HTTP/1.1 400 Bad Request\r\n\r\n";
						response_len = strlen(response);
						use_dynamic_response = 0;
					} else {
						size_t content_length = (size_t)content_length_val;
						// --- Locate Body Start --- 
						char *body_start_ptr = strstr(buffer, "\r\n\r\n");
						if (body_start_ptr == NULL) {
							fprintf(stderr, "Malformed request: Cannot find body separator \r\n\r\n\n");
							response = "HTTP/1.1 400 Bad Request\r\n\r\n";
							response_len = strlen(response);
							use_dynamic_response = 0;
						} else {
							char *body_in_buffer_start = body_start_ptr + 4;
							ssize_t initial_body_bytes_signed = bytes_received - (body_in_buffer_start - buffer);
							size_t initial_body_bytes = (initial_body_bytes_signed < 0) ? 0 : (size_t)initial_body_bytes_signed;

							// Check/handle if initial recv got more data than content-length specifies
							if (initial_body_bytes > content_length) {
								fprintf(stderr, "Warning: Received more initial data (%zu) than Content-Length (%zu), truncating file write.\n", initial_body_bytes, content_length);
								initial_body_bytes = content_length; // Only use up to content_length
							}

							// --- Write to File --- 
							FILE *file = fopen(full_path, "wb"); // Open for binary write
							if (file == NULL) {
								perror("fopen failed for writing (POST)");
								response = "HTTP/1.1 500 Internal Server Error\r\n\r\n";
								response_len = strlen(response);
								use_dynamic_response = 0;
							} else {
								size_t total_bytes_written = 0;
								int write_error = 0;

								// Write the part of the body already in the buffer
								if (initial_body_bytes > 0) {
									if (fwrite(body_in_buffer_start, 1, initial_body_bytes, file) != initial_body_bytes) {
										perror("fwrite failed for initial chunk (POST)");
										write_error = 1;
									} else {
										total_bytes_written += initial_body_bytes;
									}
								}

								// Read remaining bytes from socket and write to file
								size_t bytes_remaining = content_length - total_bytes_written;
								char chunk_buffer[4096];
								while (!write_error && bytes_remaining > 0) {
									// Determine max bytes to read in this recv call
									size_t bytes_to_read_now = (bytes_remaining < sizeof(chunk_buffer)) ? bytes_remaining : sizeof(chunk_buffer);
									ssize_t chunk_read = recv(client_fd, chunk_buffer, bytes_to_read_now, 0);

									if (chunk_read < 0) { // recv error
										perror("recv failed while reading body (POST)");
										write_error = 1; break;
									} else if (chunk_read == 0) { // Client closed connection
										fprintf(stderr, "Client disconnected before sending full body (expected %zu more bytes)\n", bytes_remaining);
										// File will be potentially partial, which is acceptable per spec.
										break; 
									}
									
									// Write the received chunk
									if (fwrite(chunk_buffer, 1, chunk_read, file) != (size_t)chunk_read) {
										perror("fwrite failed for subsequent chunk (POST)");
										write_error = 1; break;
									}
									total_bytes_written += chunk_read;
									bytes_remaining -= chunk_read; // Decrement remaining count
								}

								// Close the file regardless of write success/failure
								if (fclose(file) != 0) {
									perror("fclose failed");
									// Mark as error even if writes seemed ok, as close failed
									write_error = 1;
								}

								if (write_error) {
									// Set 500 response if not already set (e.g., by fclose)
									response = "HTTP/1.1 500 Internal Server Error\r\n\r\n";
									response_len = strlen(response);
									use_dynamic_response = 0;
								} else {
									// Success! Send 201 Created
									const char *created_response = "HTTP/1.1 201 Created\r\n\r\n";
									if (send(client_fd, created_response, strlen(created_response), 0) == -1) {
										perror("Send failed for 201 response");
										// Can't send response, but file likely created. Error is logged.
									}
									response_sent = 1; // Mark response as handled
								}
							}
						}
					}
				} 
				// --- Handle other methods for /files/ --- 
				else {
					 fprintf(stderr, "Method %s not allowed for /files/\n", method);
					 response = "HTTP/1.1 405 Method Not Allowed\r\n\r\n";
					 response_len = strlen(response);
					 use_dynamic_response = 0;
				}
			} // End path construction check
		} // End g_directory_path check
	} // End /files/ route block
	// Handle any other paths (Not Found) - This is the fallback
	else {
		// Added method check to ensure only GET gives 404 for unknown paths
		if (parsed_items == 3 && strcmp(method, "GET") != 0) {
			fprintf(stderr, "Method %s not supported for path %s\n", method, path);
			response = "HTTP/1.1 405 Method Not Allowed\r\n\r\n";
			response_len = strlen(response);
		} else {
			// Default to 404 for GET requests to unknown paths or parse errors
			response = "HTTP/1.1 404 Not Found\r\n\r\n";
			response_len = strlen(response);
		}
		use_dynamic_response = 0;
	}
	// --- End of Request Routing ---

	// Send the prepared response back to the client, but only if it wasn't handled internally
	// by a specific route (like the file serving route).
	if (!response_sent && response != NULL) {
		// Use send() to transmit data
		ssize_t bytes_sent = send(client_fd, response, response_len, 0);
		// Error handling for send
		if (bytes_sent == -1) {
			perror("Send failed");
			// Fall through to free response if needed and close socket
		} else if ((size_t)bytes_sent < response_len) {
			// Handle cases where not all data was sent (less likely for small responses)
			fprintf(stderr, "Warning: Partial send (%zd / %zu bytes)\n", bytes_sent, response_len);
		}
		
		// If the response string was dynamically allocated, free it now.
		if (use_dynamic_response) {
			free(response);
		}
	} else if (!response_sent && response == NULL) {
		// This case indicates a logic error - a route didn't prepare a response
		// nor did it send one itself.
		fprintf(stderr, "Error: Response is NULL and wasn't sent internally. Closing connection.\n");
	} else if (response_sent) {
		// Response was already sent by the handler (e.g., file transfer). No further action needed here.
		// printf("Debug: Response already sent by handler for FD %d\n", client_fd);
	}

	// Close the connection with this specific client.
	close(client_fd);
	// Thread finishes its work.
	return NULL;
}


/* Main function - entry point of the program */
int main(int argc, char *argv[]) {
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
		perror("Socket creation failed"); // Use perror for better error messages
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
		perror("SO_REUSEADDR failed");
		close(server_fd); // Close socket before exiting
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
		perror("Bind failed");
		close(server_fd);
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
		perror("Listen failed");
		close(server_fd);
		return 1; /* Exit with error code 1 */
	}
	
	// --- Argument Parsing ---
	// Look for the --directory argument
	for (int i = 1; i < argc; ++i) {
		if (strcmp(argv[i], "--directory") == 0) {
			if (i + 1 < argc) { // Make sure there is a value after the flag
				g_directory_path = argv[i + 1];
				printf("Serving files from directory: %s\n", g_directory_path);
				break; // Stop parsing once directory is found
			} else {
				fprintf(stderr, "Error: --directory flag requires an argument.\n");
				close(server_fd);
				return 1; // Exit if argument is malformed
			}
		}
	}
	// Optional: Could add a check here to ensure g_directory_path is set if required
	// if (g_directory_path == NULL) { ... error ... }

	printf("Waiting for clients to connect...\n");
	
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
	// --- Main Server Loop ---
	// Continuously accept incoming connections
	while(1) {
		client_addr_len = sizeof(client_addr); // Reset size for each accept call
		// Accept a new connection. This blocks until a client connects.
		int client_fd = accept(server_fd, (struct sockaddr *)&client_addr, &client_addr_len);
		if (client_fd < 0) {
			perror("Accept failed");
			// Don't exit the server on accept failure, just log and continue.
			continue; 
		}
		
		// Log client connection
		printf("Client connected (FD: %d)\n", client_fd);

		// --- Thread Creation ---
		// We need to pass the client_fd to the new thread. 
		// Since client_fd is on the stack of this loop and the loop continues,
		// we must allocate memory on the heap to safely pass the FD value.
		int *client_fd_arg = malloc(sizeof(int));
		if (client_fd_arg == NULL) {
			perror("Failed to allocate memory for thread argument");
			close(client_fd); // Close the accepted socket as we can't handle it
			continue; // Continue to the next loop iteration
		}
		*client_fd_arg = client_fd; // Store the accepted file descriptor

		// Create a new thread to handle the client connection.
		pthread_t thread_id;
		if (pthread_create(&thread_id, NULL, handle_client, client_fd_arg) != 0) {
			perror("pthread_create failed");
			free(client_fd_arg); // Free allocated memory as thread creation failed
			close(client_fd);    // Close the client socket
			continue; // Continue to the next loop iteration
		}

		// Detach the thread. This tells the system that we don't intend to 
		// join this thread later. Resources associated with the thread will be 
		// automatically released when the thread terminates. This simplifies cleanup.
		pthread_detach(thread_id);

		// The main loop immediately continues to accept the next connection, 
		// while the newly created thread handles the previous client concurrently.
	} // --- End of Main Server Loop ---
	
	/* 
	 * The code below is now unreachable because of the infinite `while(1)` loop. 
	 * In a production server, you'd typically have signal handling (e.g., for Ctrl+C) 
	 * to gracefully break the loop and close the main server socket.
	 * For this exercise, leaving the loop infinite is acceptable.
	 */
	// close(server_fd);

	return 0; // Technically unreachable, but good practice.
}