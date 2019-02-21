/**
 * client.c
 *
 * @author Taylor Wong
 * USD COMP 375: Computer Networks
 * Project 1: Handles communication over a network between a client and server
 * (comp375.sandiego.edu) using sockets.  This program implements the client
 * (sensor.sandiego.edu) and handles communication with the server based on
 * the sensor chosen by the user.
 *
 */

#define _XOPEN_SOURCE 600
#define BUFF_SIZE 1024

#include <errno.h>
#include <limits.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <time.h>

long prompt();
int connectToHost(char *hostname, char *port);
void mainLoop(int server_fd);
void send_or_exit(int fd, char *buff, size_t buff_len);
void recv_or_exit(int fd, char *buff, size_t max_len);
char* parse_port(char *str);
char* protocol(int server_fd, char *sensor_opt);

int main() {
	int server_fd = connectToHost("comp375.sandiego.edu", "47789");
	mainLoop(server_fd);
	close(server_fd);
	return 0;
}

/**
 * Loop to keep asking user what they want to do and calling the appropriate
 * function to handle the selection.
 *
 * @param server_fd Socket file descriptor for communicating with the server
 */
void mainLoop(int server_fd) {

	printf("WELCOME TO THE COMP375 SENSOR NETWORK\n\n\n");
	char *response, *date;
	long int epoch;
	int condition;
	char *sensor;

	while (1) {
		long selection = prompt();
		server_fd = connectToHost("comp375.sandiego.edu", "47789");
		switch (selection) {
			case 1: // air temperature
				sensor = "AIR TEMPERATURE";
				response = protocol(server_fd, "AIR TEMPERATURE\n");
				sscanf(response, "%ld %d F", &epoch, &condition);  // parse msg 
				date = ctime(&epoch);   // convert epoch to readable date
				printf("\nThe last %s reading was %d F, taken at %s\n", sensor, condition, date);
				free(response);
				break;
			case 2: // Relative humidity
				sensor = "RELATIVE HUMIDITY";
				response = protocol(server_fd, "RELATIVE HUMIDITY\n");
				sscanf(response, "%ld %d ", &epoch, &condition);
				date = ctime(&epoch);
				printf("\nThe last %s reading was %d %%, taken at %s\n", sensor, condition, date);
				free(response);
				break;
			case 3: // wind speed
				sensor = "WIND SPEED";
				response = protocol(server_fd, "WIND SPEED");
				sscanf(response, "%ld %d MPH", &epoch, &condition);
				date = ctime(&epoch);
				printf("\nThe last %s reading was %d MPH, taken at %s\n", sensor, condition, date);
				free(response);
				break;
			case 4: // quit
				printf("GOODBYE!\n");
				exit(0);
				break;
			default:
				fprintf(stderr, "ERROR: Invalid selection\n");
				break;
		}
	}
}

/**
 * Parses the message received from the server to get the port number
 * 
 * @param buff is the string message to be parsed
 * @returns the port number
 */
char* parse_port(char *buff){
	char delim[] = " ";
	char * token = strtok(buff, delim); // CONNECT
	token = strtok(NULL, delim); // sensor.sandiego.edu
	token = strtok(NULL, delim); // port number
	return token;
}


/**
 * Handles communication between a client and server over a network
 *
 * @param server_fd is the socket descriptor of the server
 * @param sensor_opt is an indicator of the type of sensor chosen
 * @returns the response from the server containing info read from the
 * specified sensor
 */
char* protocol(int server_fd, char *sensor_opt){ 
	char buff[BUFF_SIZE];
	memset(buff, 0, BUFF_SIZE);

	// Authorize Server
	send_or_exit(server_fd, "AUTH password123\n", 17);
	recv_or_exit(server_fd, buff, BUFF_SIZE); 

	// parse into port number and server name
	char *port_num = parse_port(buff);	

	// Connect to Sensor host with port number
	int sensor_fd = connectToHost("sensor.sandiego.edu", port_num);  

	char result[BUFF_SIZE];
	memset(result, 0, BUFF_SIZE);
	send_or_exit(sensor_fd, "AUTH sensorpass321\n", 19);
	recv_or_exit(sensor_fd, result, BUFF_SIZE);

	// Read in Sensor Data
	char *response = malloc(sizeof(char)*1024);
	send_or_exit(sensor_fd, sensor_opt, strlen(sensor_opt));
	recv_or_exit(sensor_fd, response, BUFF_SIZE);

	// Close Sensor Connection
	char msg[BUFF_SIZE];
	memset(msg, 0, BUFF_SIZE);
	send_or_exit(sensor_fd, "CLOSE\n", 6);
	recv_or_exit(sensor_fd, msg, BUFF_SIZE);
	close(sensor_fd);

	return response;
}

/** 
 * Print command prompt to user and obtain user input.
 *
 * @return The user's desired selection, or -1 if invalid selection.
 */
long prompt() {
	// Print the sensor options for the user
	printf("Which sensor would you like to read: \n");
	printf("\n\t(1) Air temperature \n");
	printf("\t(2) Relative humidity \n");
	printf("\t(3) Wind speed \n");
	printf("\t(4) Quit Program \n");
	printf("\nSelection: ");

	// Read in a value from standard input
	char input[10];
	memset(input, 0, 10); // set all characters in input to '\0' (i.e. nul)
	char *read_str = fgets(input, 10, stdin);

	// Check if EOF or an error, exiting the program in both cases.
	if (read_str == NULL) {
		if (feof(stdin)) {
			exit(0);
		}
		else if (ferror(stdin)) {
			perror("fgets");
			exit(1);
		}
	}

	// get rid of newline, if there is one
	char *new_line = strchr(input, '\n');
	if (new_line != NULL) new_line[0] = '\0';

	// convert string to a long int
	char *end;
	long selection = strtol(input, &end, 10);

	if (end == input || *end != '\0') {
		selection = -1;
	}

	return selection;
}

/**
 * Socket implementation of connecting to a host at a specific port.
 *
 * @param hostname The name of the host to connect to (e.g. "foo.sandiego.edu")
 * @param port The port number to connect to
 * @return File descriptor of new socket to use.
 */
int connectToHost(char *hostname, char *port) {
	// Step 1: fill in the address info in preparation for setting up the socket
	int status;
	struct addrinfo hints;
	struct addrinfo *servinfo;  // will point to the results

	memset(&hints, 0, sizeof hints); // make sure the struct is empty
	hints.ai_family = AF_INET;       // Use IPv4
	hints.ai_socktype = SOCK_STREAM; // TCP stream sockets

	// get ready to connect
	if ((status = getaddrinfo(hostname, port, &hints, &servinfo)) != 0) {
		fprintf(stderr, "getaddrinfo error: %s\n", gai_strerror(status));
		exit(1);
	}

	// Step 2: Make a call to socket
	int fd = socket(servinfo->ai_family, servinfo->ai_socktype, servinfo->ai_protocol);
	if (fd == -1) {
		perror("socket");
		exit(1);
	}

	// Step 3: connect!
	if (connect(fd, servinfo->ai_addr, servinfo->ai_addrlen) != 0) {
		perror("connect");
		exit(1);
	}

	freeaddrinfo(servinfo); // free's the memory allocated by getaddrinfo

	return fd;
}

/*
 * Sends message over given socket or exits if something went wrong
 *
 * @param fd is the socket descriptor you want to send data to
 * @param buff is a pointer to the data you want to send
 * @param buff_len is the length of that data in bytes
 */
void send_or_exit(int fd, char *buff, size_t buff_len){
	// sent is the number of bytes actually sent out
	int sent = send(fd, buff, buff_len, 0);
	if(sent == 0){
		printf("Server connection closed unexpectedly. Goodbye \n");
		exit(1);
	}
	else if(sent == -1){
		perror("send");
		exit(1);
	}
	if(sent < (int) buff_len){
		int new_len = (int) buff_len - sent;
		char sub_buff[new_len];
		memcpy(sub_buff, &buff[sent], new_len);
		send_or_exit(fd, sub_buff, new_len);
	}
}

/*
 * Receives message from given socket or exits if something went wrong
 *
 * @param fd is the socket descriptor to read from
 * @param buff is the buffer to read the information into
 * @param max_len is the maximum length of the buffer
 */
void recv_or_exit(int fd, char *buff, size_t max_len){
	int recvd = recv(fd, buff, max_len, 0);
	if(recvd == 0){
		printf("Server connetion closed unexpectedly. Goodbye \n");
		exit(1);
	}
	else if(recvd == -1){
		perror("recv");
		exit(1);
	}
}
