#include <sys/wait.h> // for waitpid
#include <stdio.h>    // for printf and perror
#include <stdlib.h>   // for exit
#include <unistd.h>   // for execv, getpid, fork
#include <string.h>   // string functions

int main(){
	//Struct to contain user command
	struct usrInput
	{
		char *command;
		char *x;
		char *y;
		char *z;
	};
	
	//Iterate as shell
	while(1) {

		//function variables
		int childStatus;
		char scanInput[40];

		//display prompt..get command
		printf(": ");
		scanf("%s", scanInput);

		//create struct allocate space
		struct usrInput *currInput = malloc(sizeof(struct usrInput));
		
		//create property of struct and allocate space with calloc for diverse use
		currInput->command = calloc(strlen(scanInput) + 1, sizeof(char));
 		strcpy(currInput->command, scanInput);
		
		// Fork a new process
		pid_t spawnPid = fork();

		//Switch between parent and child process
		switch(spawnPid){

		//if fails to spawn
		case -1:
			perror("error in fork\n");
			exit(1);
			break;
		
		//if child process
		case 0:
			
			printf("user input now: %s\n", currInput->command);
			exit(2);
			break;

		//parent process
		default:
			//wait for child to complete
			spawnPid = waitpid(spawnPid, &childStatus, 0);
			
		} 
	}
}