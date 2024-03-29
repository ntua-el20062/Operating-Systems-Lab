#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/wait.h>

#include "lunix-chrdev.h"

#define FILE_PATH "/dev/lunix0-temp"
#define BUFFER_SIZE 1024
#define NUM_CHILDREN 200
void readFromFile(int childNumber, int fd) {
	int file_descriptor=fd;
	char buffer[BUFFER_SIZE];

	// Open the file with read-only access


	if (file_descriptor == -1) {
    	perror("Error opening file");
    	exit(EXIT_FAILURE);
	}

	// Read and print the contents of the file
	ssize_t bytesRead = read(file_descriptor, buffer, sizeof(buffer) - 1);

	if (bytesRead == -1) {
    	perror("Error reading file");
    	close(file_descriptor);
    	exit(EXIT_FAILURE);
	}

	buffer[bytesRead] = '\0'; // Null-terminate the string

	printf("Child %d - Content of %s: %s\n", childNumber, FILE_PATH, buffer);

	// Close the file descriptor
	close(file_descriptor);
}

int main() {
	pid_t pid;
	int fd = open(FILE_PATH, O_RDONLY);

	for (int i = 1; i <= NUM_CHILDREN; ++i) {
    	pid = fork();

    	if (pid == -1) {
        	perror("Error forking");
        	exit(EXIT_FAILURE);
    	} else if (pid == 0) {
        	// Child process
        	readFromFile(i, fd);
        	exit(EXIT_SUCCESS);
    	}

	}

	// Parent process waits for all child processes to finish
	for (int i = 0; i < NUM_CHILDREN; ++i) {
    	wait(NULL);
	}

	return 0;
}

