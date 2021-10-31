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
		int background;
		int redirectOut;
		int redirectIn;
		int commandArraySize;
		char *commandArray[512];
	};

struct shellSession
	{
		int *lastForegroundPid;
		int *pidArraySize;
		int *pidArray[40];
	};

//parses input token and updates argument pointer
struct usrInput *parseInput(char *currLine){

	//create struct allocate space
	struct usrInput *currInput = malloc(sizeof(struct usrInput));

	// Exctract command
		
		//convert $$ to PID
		char expand[10];
		char newToken[40] = "";
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
		currInput->background = 0;
		currInput->redirectOut = 0;
		currInput->redirectIn = 0;
		currInput->argument = calloc(strlen(currLine)+40, sizeof(char));
		

		//iterate through command line and add to argument list
			for (int j = 0;j < i; j++) {

				if (strcmp(currInput->commandArray[j], ">") == 0) {
					currInput->redirectOut = j + 1;
				};

				if (strcmp(currInput->commandArray[j], "<") == 0) {
					currInput->redirectIn = j + 1;
				};
				
				if (strcmp(currInput->commandArray[j], "&") == 0) {
					currInput->background = j + 1;

					if (j+1 == i) {
						break;
					}
				};

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
	
	struct shellSession *currSession = malloc(sizeof(struct shellSession));
	
	currSession->lastForegroundPid = calloc(4, sizeof(int));
	currSession->pidArraySize = calloc(4, sizeof(int));

	int looper = 0;
	
	//Iterate as shell
	while(1) {
		looper++;
		
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
		
		if (strcmp(parsedInput->command, "exit")==0){
				
				//Clean up by freeing, then continue
				free(parsedInput->command);
				for(int j=0;j<parsedInput->commandArraySize;j++) {
					free(parsedInput->commandArray[j]);
				};
				free(parsedInput->argument);
				free(parsedInput);

				free(currSession->lastForegroundPid);
				free(currSession); 
				
				kill(getpid(), SIGTERM);
				
			};

		if (strcmp(parsedInput->command, "cd")==0){
			 
			
			if (strlen(parsedInput->argument) == 0) {
			chdir(getenv("HOME"));
			}else{
			chdir(parsedInput->argument);	
			}
			
			//Clean up by freeing, then continue
			free(parsedInput->command);
			for(int j=0;j<parsedInput->commandArraySize;j++) {
				free(parsedInput->commandArray[j]);
			};
			free(parsedInput->argument);
			free(parsedInput);
			continue;
			
			
		};

		//if command was status
				if (strcmp(parsedInput->command, "status")==0){

					if (looper == 1) {
						printf("Looper: Exit status %d \n", WEXITSTATUS(0));
					}
					else {
			
					if(WIFEXITED(childStatus)){
						printf("Exit status %d\n", WEXITSTATUS(childStatus));
						fflush(stdout);
					} else{
						printf("Terminated by signal %d\n", WTERMSIG(childStatus));
						fflush(stdout);
					}
					}

					//Clean up by freeing, then continue
					free(parsedInput->command);
					for(int j=0;j<parsedInput->commandArraySize;j++) {
						free(parsedInput->commandArray[j]);
					};
					free(parsedInput->argument);
					free(parsedInput);
					continue;
				};
		

		

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


			//if user didn't redirect stdout
			//This helped me: https://stackoverflow.com/questions/14846768/in-c-how-do-i-redirect-stdout-fileno-to-dev-null-using-dup2-and-then-redirect  		
			if ((parsedInput->redirectOut == 0) && (parsedInput->background > 0)) {
				fflush(stdout);
				int toDevNullOut = open("/dev/null", O_WRONLY);
				dup2(toDevNullOut, STDOUT_FILENO);
				};

			//if user didn't redirect stdin
			if ((parsedInput->redirectIn == 0) && (parsedInput->background > 0)) {
				int toDevNullIn = open("/dev/null", O_RDWR);
				dup2(toDevNullIn, STDIN_FILENO);
				};


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
            			printf("cannot open %s for input\n", fileName);
            			
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
					
					//run command, pass in args
					if ((execlp(parsedInput->command, parsedInput->command, parsedInput->argument, NULL)) == -1) {
						perror("");
						fflush(stdout);
						exit(1);
					};
				}}

			// parsedInput->commandArray[parsedInput->commandArraySize] = NULL;
			
			// for (int k =0; k < parsedInput->commandArraySize; k++) {
			// 	printf("%s\n", parsedInput->commandArray[k]);
			// 	fflush(stdout);
			// }
			
			// execvp(parsedInput->command, parsedInput->commandArray);

			//if there werew no arguments passed, set to null for exec() use
			if (strlen(parsedInput->argument) == 0) {parsedInput->argument = NULL;};

				//run command, pass in args
			if ((execlp(parsedInput->command, parsedInput->command, parsedInput->argument, NULL)) == -1) {
					
					perror("");
					fflush(stdout);
					exit(1);
				}
			
				exit(0);
				
				
			
			
			// //break;

		//parent process
		default:

		for(int j=0;j<*currSession->pidArraySize;j++) {

			printf("%d", waitpid(*currSession->pidArray[j], &childStatus, WNOHANG));

				if (waitpid(*currSession->pidArray[j], &childStatus, WNOHANG) != 0 ) {
					printf("child %d is done", *currSession->pidArray[j]);
					fflush(stdout);
					//delete from array
					free(currSession->pidArray[j]);
					currSession->pidArray[j] = currSession->pidArray[j+1];
					
					*currSession->pidArraySize = *currSession->pidArraySize - 1;
				};
				//printf("%d", *currSession->pidArray[j]);
				fflush(stdout);
			};

		
		if (parsedInput->background == 0) {
				
				*currSession->lastForegroundPid = spawnPid;
				
			}

			//Handle background processing
			if (parsedInput->background > 0) {
					
					
				
				currSession->pidArray[*currSession->pidArraySize] = calloc(4, sizeof(int));
				*currSession->pidArray[*currSession->pidArraySize] = spawnPid;
				*currSession->pidArraySize = *currSession->pidArraySize + 1;

				spawnPid = waitpid(spawnPid, &childStatus, WNOHANG);
				printf("background pid %d is done: exit value %d\n", spawnPid, WEXITSTATUS(childStatus));
				fflush(stdout);
					
					
				}
			else{
				//wait for child to complete
				spawnPid = waitpid(spawnPid, &childStatus, 0);

			}

			free(parsedInput->command);
	
			for(int j=0;j<parsedInput->commandArraySize;j++) {
				free(parsedInput->commandArray[j]);
			};
			
			free(parsedInput->argument);
			
			free(parsedInput);
			
			break;
		}
		
		
	
	}
	free(&currSession->pidArraySize);
	for(int j=0;j<*currSession->pidArraySize;j++) {
				free(currSession->pidArray[j]);
			};
	
	free(currSession->lastForegroundPid);
	free(currSession); 
	
	return 0;
}