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
		char *argument;
		char *z;
		char *commandArray[40];
	};
	
	
	//Iterate as shell
	while(1) {

		//function variables
		int childStatus;
		int secondChildStatus;
		char scanInput[40];
		char scanInput1[40];

		//display prompt..get command
		//ran into an issue with reading a sentence this resource help: https://stackoverflow.com/questions/18547004/how-to-scanf-full-sentence-in-c
		printf(": ");
		fflush(stdout);
		fgets(scanInput,40,stdin);
		if (sscanf(scanInput, "%s", scanInput1) == -1){
			
			continue;
			}

		//create struct allocate space
		struct usrInput *currInput = malloc(sizeof(struct usrInput)+ 256);
		
		// Exctract command
		int i = 0;
		char *saveptr;
		strcpy(scanInput1, scanInput);

		//convert $$ to PID
		char expand[10];
		char newToken[256] = "";
		char gotPid[40];

		
		//convert $$ to PID if found
		for(int j =1; j<strlen(scanInput);j++) {
			char one = scanInput[j-1];
			char two = scanInput[j];
			sprintf(expand, "%c%c", scanInput[j-1], scanInput[j]);
			if(strcmp(expand, "$$")==0){
				sprintf(gotPid, "%d", getpid());
				strcat(newToken, gotPid);
				j++;
				continue;
			}
			//printf("%c \n", one);
			// = "yo";
			strncat(newToken, &one, 1);

			if(j==strlen(scanInput)-1) {
				strncat(newToken, &two, 1);
			}
			
		}
		
		strcpy(scanInput, newToken);
		


		//Set command
		char *token = strtok_r(scanInput, " \n", &saveptr);


		
		currInput->command = calloc(strlen(token)+20, sizeof(char));
		
 		strcpy(currInput->command, token);
		
		//currInput->commandArray = calloc(40, sizeof(char));
		
		//Set argument array
		while(1) {
			
			token = strtok_r(NULL, " \n", &saveptr);
			
			if (token==NULL) {
				break;
			};
			
			if (token!=NULL) {
				
			currInput->commandArray[i] = calloc(strlen(token)+1, sizeof(char));
			strcpy(currInput->commandArray[i], token);
			i++;

			}
			
		}
		
		

		// Fork a new process
		pid_t spawnPid = fork();

		//set argument string
		char space[] = " ";
		char empty[] = "";
		
		//Switch between parent and child process
		switch(spawnPid){

		//if fails to spawn
		case -1:
			perror("error in fork\n");
			fflush(stdout);
			exit(1);
			break;
		
		//if child process
		case 0:

			//Allocate space to argument
			currInput->argument = calloc(strlen(scanInput)+40, sizeof(char));
			

			//redirect stout first if needed
			for (int j =0;j < i;j++){
				//Argument is a redirect to out
				if (strcmp(currInput->commandArray[j], ">")==0){
					char* fileName = currInput->commandArray[j+1];
					//printf("%s", currInput->argument);
					int fdOut = open(fileName, O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR);

					if (fdOut == -1){
            			printf("open() failed on \"%s\"\n", fileName);
            			perror("Error");
						fflush(stdout);
            			exit(1);
        			}
					
					dup2(fdOut, STDOUT_FILENO);
					
					fflush(stdout);
					
					break;
					
				}
			}
			
			//iterate through command line and add to argument list
			for (int j = 0;j < i; j++) {

				//add space after each argument
				if (j > 0) {
					strcat(currInput->argument, space);
				};
				
				//If redirect out: already handled..break
				//Do not remove.. will add unwanted stdout
				if (strcmp(currInput->commandArray[j], ">")==0){
					break;
					//continue;
				}

				//argument is a redirect from in 
				if (strcmp(currInput->commandArray[j], "<")==0){

					char* fileName = currInput->commandArray[j+1];
					
					int fdIn = open(fileName, O_RDONLY);

					if (fdIn == -1){
            			printf("open() failed on \"%s\"\n", fileName);
            			perror("Error");
						fflush(stdout);
            			exit(1);
        			}
					
					
					dup2(fdIn, STDIN_FILENO);

					if (strlen(currInput->argument) == 0) {
						currInput->argument = NULL;
						};

					pid_t secondSpawnPid = fork();
					
					//Switch between parent and child process
					switch(secondSpawnPid){
						//if fails to spawn
						case -1:
							perror("error in fork\n");
							fflush(stdout);
							exit(1);
							break;

						//if child process
						case 0:
						execlp(currInput->command, currInput->command, currInput->argument, NULL);

						//parent process
						default:

						//wait for child to complete
						secondSpawnPid = waitpid(secondSpawnPid, &secondChildStatus, 0);
						close(fdIn);
						
						exit(1);
						
					};
					
					
					
				}
				//if argument was set to NULL set back to empty str for strcat
				if (!currInput->argument) {
						currInput->argument = empty;
						};

				
				//add argument to string
				strcat(currInput->argument, currInput->commandArray[j]);
				
			}	
			
			//if there werew no arguments passed, set to null for exec() use
			if (strlen(currInput->argument) == 0) {
				currInput->argument = NULL;
				
			};
			// printf("%s hi", currInput->argument);
			// fflush(stdout);
			execlp(currInput->command, currInput->command, currInput->argument, NULL);
			
			
			break;

		//parent process
		default:
			//wait for child to complete
			spawnPid = waitpid(spawnPid, &childStatus, 0);
			free(currInput);
			
			break;
		} 
		
	}
	
	
	return 0;
}