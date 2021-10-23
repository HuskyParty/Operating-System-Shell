#include <sys/wait.h> // for waitpid
#include <stdio.h>    // for printf and perror
#include <stdlib.h>   // for exit
#include <unistd.h>   // for execv, getpid, fork
#include <string.h>   // string functions
#include <fcntl.h>

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
		char scanInput1[40];

		//display prompt..get command
		//ran into an issue with reading a sentence this resource help: https://stackoverflow.com/questions/18547004/how-to-scanf-full-sentence-in-c
		printf(": ");
		
		fgets(scanInput,40,stdin);
		if (sscanf(scanInput, "%s", scanInput1) == -1){
			
			continue;
			}


		
		//create struct allocate space
		struct usrInput *currInput = malloc(sizeof(struct usrInput));
		
		// Exctract command
		int i = 0;
		char *saveptr;

		//Set command
		char *token = strtok_r(scanInput, " \n", &saveptr);
		
		
		currInput->command = calloc(strlen(scanInput) + 1, sizeof(char));
 		strcpy(currInput->command, token);
		
		
		//Set argument array
		while(1) {
			
			token = strtok_r(NULL, " \n", &saveptr);
			
			if (token==NULL) {
				break;
			}

			
			
			currInput->commandArray[i] = calloc(strlen(scanInput) + 1, sizeof(char));
			strcpy(currInput->commandArray[i], token);
			//printf("%s", currInput->commandArray[i]);
			i++;
			
		}

		
		//printf(currInput->argument);
		// Fork a new process
		pid_t spawnPid = fork();
		//set argument string
			char space[] = " ";
		//Switch between parent and child process
		switch(spawnPid){

		//if fails to spawn
		case -1:
			perror("error in fork\n");
			exit(1);
			break;
		
		//if child process
		case 0:

			

			currInput->argument = calloc(strlen(scanInput) + 1, sizeof(char));
		
			for (int j = 0;j < i; j++) {

				//add space after each argument
				if (j > 0) {
					strcat(currInput->argument, space);
				};

				if (strcmp(currInput->commandArray[j], ">")==0){
					char* fileName = currInput->commandArray[j+1];
					//printf("%s", currInput->argument);
					int fd = open(fileName, O_WRONLY | O_CREAT | O_TRUNC);

					if (fd == -1){
            			printf("open() failed on \"%s\"\n", fileName);
            			perror("Error");
            			exit(1);
        			}
					//FILE *fd = fopen(fileName, "w");
					dup2(fd, STDOUT_FILENO);
					printf("found ya");
					break;
					//continue;
				}
				//add argument to string
				strcat(currInput->argument, currInput->commandArray[j]);
				
			}	
			printf("%s", currInput->argument);
			//printf("%s\n", currInput->argument);
			//if there werew no arguments passed, set to null for exec() use
			if (strlen(currInput->argument) == 0) {
				currInput->argument = NULL;
			};
			
			execlp(currInput->command, currInput->command, currInput->argument, NULL);
			
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