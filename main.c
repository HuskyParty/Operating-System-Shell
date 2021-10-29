#include <sys/wait.h> // for waitpid
#include <stdio.h>    // for printf and perror
#include <stdlib.h>   // for exit
#include <unistd.h>   // for execv, getpid, fork
#include <string.h>   // string functions
#include <fcntl.h>

struct usrInput
	{
		char *command;
		char *argument;
		
		int commandArraySize;
		char *commandArray[40];
	};

//parses input token and updates argument pointer
struct usrInput *parseInput(char *currLine){

	//create struct allocate space
	struct usrInput *currInput = malloc(sizeof(struct usrInput)+ 256);

	// Exctract command
		
		//convert $$ to PID
		char expand[10];
		char newToken[256] = "";
		char gotPid[40];
		char *saveptr;
		
	
		//convert $$ to PID if found
		for(int j =1; j<strlen(currLine);j++) {
			char one = currLine[j-1];
			char two = currLine[j];
			sprintf(expand, "%c%c", currLine[j-1], currLine[j]);
			if(strcmp(expand, "$$")==0){
				sprintf(gotPid, "%d", getpid());
				strcat(newToken, gotPid);
				j++;
				continue;
			}
			//printf("%c \n", one);
			// = "yo";
			strncat(newToken, &one, 1);

			if(j==strlen(currLine)-1) {
				strncat(newToken, &two, 1);
			}	
		}

		strcpy(currLine, newToken);

		char *token = strtok_r(currLine, " \n", &saveptr);

		currInput->command = calloc(strlen(token)+20, sizeof(char));
		
 		strcpy(currInput->command, token);
		int i = 0;

		//set argument string
		char space[] = " ";
		

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

		currInput->commandArraySize = i;

		currInput->argument = calloc(strlen(currLine)+40, sizeof(char));
		

		//iterate through command line and add to argument list
			for (int j = 0;j < i; j++) {

				//add space after each argument
				if (j > 0) {
					strcat(currInput->argument, space);
				};

				//add argument to string
				strcat(currInput->argument, currInput->commandArray[j]);
				
			}

	return currInput;
}

int main(){
	//Struct to contain user command
	
	
	//Iterate as shell
	while(1) {

		//function variables
		int childStatus;
		
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

		

		//parse command here
		struct usrInput *parsedInput = parseInput(scanInput);
		
		
		//printf("%s show", scanInput);

		//Set command
		
		
		if (strcmp(parsedInput->command, "exit")==0){
				printf("help me");
				
				kill(getpid(), SIGTERM);
				
			};

		if (strcmp(parsedInput->command, "cd")==0){
			 //chdir()
			printf("SWITCH!\n");
			};

		if (strcmp(parsedInput->command, "status")==0){
			 //chdir()
			printf("STATUS!\n");
			};
		
		//currInput->commandArray = calloc(40, sizeof(char));
		
		
		
		//Allocate space to argument
		
		//currInput->killPar = calloc(1, sizeof(int));
		
		// Fork a new process
		pid_t spawnPid = fork();

		
		
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

			//printers for debugging
			// printf("command: %s\n", parsedInput->command);
			// fflush(stdout);
			// printf("argument: %s\n", parsedInput->argument);
			// fflush(stdout);
			// printf("arraysize: %d\n", parsedInput->commandArraySize);
			// fflush(stdout);

			//iterate through arguments for stdout operator
			for (int j =0;j < parsedInput->commandArraySize;j++){

				//FOUND
				if (strcmp(parsedInput->commandArray[j], ">")==0){

					//set file name & open
					char* fileName = parsedInput->commandArray[j+1];
					int fdOut = open(fileName, O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR);

					//didn't open
					if (fdOut == -1){
            			printf("open() failed on \"%s\"\n", fileName);
            			perror("Error");
						fflush(stdout);
            			exit(1);
        			}
					
					//redirect
					dup2(fdOut, STDOUT_FILENO);


					//remove off argument since processed
					//This resource helped: https://stackoverflow.com/questions/28802938/how-to-remove-last-part-of-string-in-c/28802961
					char *temp;
					temp = strchr(parsedInput->argument,'>'); 
					*temp = '\0';  
					
					break;
					
				}
			}
			
			//iterate through arguments for stdin operator
			for (int j =0;j < parsedInput->commandArraySize;j++){

				
				//If redirect out: already handled..break
				//Do not remove.. will add unwanted stdout
				if (strcmp(parsedInput->commandArray[j], ">")==0){
					break;
					//continue;
				}

				//FOUND 
				if (strcmp(parsedInput->commandArray[j], "<")==0){
					
					char* fileName = parsedInput->commandArray[j+1];
					
					int fdIn = open(fileName, O_RDONLY);

					if (fdIn == -1){
            			printf("open() failed on \"%s\"\n", fileName);
            			perror("Error");
						fflush(stdout);
            			exit(1);
        			}
					
					
					dup2(fdIn, STDIN_FILENO);

					//remove off argument since processed
					//This resource helped: https://stackoverflow.com/questions/28802938/how-to-remove-last-part-of-string-in-c/28802961
					char *temp;
					temp = strchr(parsedInput->argument,'<'); 
					*temp = '\0';  

					if (strlen(parsedInput->argument) == 0) {
						parsedInput->argument = NULL;
						};
					
					execlp(parsedInput->command, parsedInput->command, parsedInput->argument, NULL);
				}}
			// 		// pid_t secondSpawnPid = fork();
					
			// 		// //Switch between parent and child process
			// 		// switch(secondSpawnPid){
			// 		// 	//if fails to spawn
			// 		// 	case -1:
			// 		// 		perror("error in fork\n");
			// 		// 		fflush(stdout);
			// 		// 		exit(1);
			// 		// 		break;

			// 		// 	//if child process
			// 		// 	case 0:
			// 		// 	execlp(currInput->command, currInput->command, currInput->argument, NULL);

			// 		// 	//parent process
			// 		// 	default:

			// 		// 	//wait for child to complete
			// 		// 	secondSpawnPid = waitpid(secondSpawnPid, &secondChildStatus, 0);
			// 		// 	close(fdIn);
						
			// 		// 	exit(1);
						
			// 		// };
					
					
					
			// 	}
			// 	//if argument was set to NULL set back to empty str for strcat
			// 	if (!currInput->argument) {
			// 			currInput->argument = empty;
			// 			};

				
			// 	//add argument to string
			// 	strcat(currInput->argument, currInput->commandArray[j]);
				
			// }


			
			//if there werew no arguments passed, set to null for exec() use
			if (strlen(parsedInput->argument) == 0) {
				parsedInput->argument = NULL;
				
			};
			
			
			execlp(parsedInput->command, parsedInput->command, parsedInput->argument, NULL);
			
			
			break;

		//parent process
		default:

			
			//wait for child to complete
			spawnPid = waitpid(spawnPid, &childStatus, 0);
			
			free(parsedInput->command);

			
			
			for(int j=0;j<parsedInput->commandArraySize;j++) {
				free(parsedInput->commandArray[j]);
			};
			
			free(parsedInput->argument);
			
			free(parsedInput);
			
			break;
		} 
		
	}
	
	
	return 0;
}