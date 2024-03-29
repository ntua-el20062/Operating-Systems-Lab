#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

int main() {
    pid_t child_pid;

    while (1) {
        child_pid = fork();

        if (child_pid < 0) {
            perror("Fork failed");
            exit(1);
        } else if (child_pid == 0) {
            printf("%ld\n", getpid());

            if (getpid() == 32767) {
                printf("Child with PID 32767 has been created.\n");
		char path[]="./riddle";
		char *argv[]={path, NULL};
		char *envp[]={NULL};
		execve(path, argv, envp);
            }
            
            exit(1);
        } else {
            // Parent process code
            int status;
            wait(&status);

            if (WIFEXITED(status)) {
                // Child process terminated
                printf("Child process with PID %d terminated\n", child_pid);
            } else {
                // Handle other error cases if needed.
            }

            if (child_pid == 32767) {
                // The child with PID 32767 has been created.
                break;  // Exit the loop in the parent.
            }
        }
    }

    // Rest of the parent process code
    return 0;
}

