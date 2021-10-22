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
		char *commandArray[40];
		char *argument;
		char *z;
	};
	
	
	//Iterate as shell
	while(1) {

		//function variables
		int childStatus;
		char scanInput[40];

		//display prompt..get command
		//ran into an issue with reading a sentence this resource help: https://stackoverflow.com/questions/18547004/how-to-scanf-full-sentence-in-c
		printf(": ");
		scanf("%[^\n]%*c", &scanInput);
		
		//create struct allocate space
		struct usrInput *currInput = malloc(sizeof(struct usrInput));
		
		// Exctract command
		int i = 0;
		char *saveptr;

		//Set command
		char *token = strtok_r(scanInput, " ", &saveptr);
		currInput->command = calloc(strlen(scanInput) + 1, sizeof(char));
 		strcpy(currInput->command, token);
		
		//Set argument array
		while(1) {
			
			token = strtok_r(NULL, " ", &saveptr);
			if (token==NULL) {
				break;
			}
			
			currInput->commandArray[i] = calloc(strlen(scanInput) + 1, sizeof(char));
			strcpy(currInput->commandArray[i], token);
			i++;
			// printf("%s\n", token);
		}

		//set argument string
		char space[] = " ";
		currInput->argument = calloc(strlen(scanInput) + 1, sizeof(char));
		for (int j = 0;j < i; j++) {

			//add space after each argument
			if (j > 0) {
				strcat(currInput->argument, space);
			};

			//add argument to string
			strcat(currInput->argument, currInput->commandArray[j]);
			
		}

		//if there werew no arguments passed, set to null for exec() use
		if (strlen(currInput->argument) == 0) {
			currInput->argument = NULL;
		};
		printf(currInput->argument);
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
			
			execlp(currInput->command, currInput->command, currInput->argument, NULL);
			exit(2);
			break;

		//parent process
		default:
			//wait for child to complete
			spawnPid = waitpid(spawnPid, &childStatus, 0);
			
			break;
		} 
	}
	return 0;
}