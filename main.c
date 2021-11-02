#include <sys/wait.h> // for waitpid
#include <stdio.h>    // for printf and perror
#include <stdlib.h>   // for exit
#include <unistd.h>   // for execv, getpid, fork
#include <string.h>   // string functions
#include <fcntl.h>
#include <signal.h>


struct usrInput
	{
		char *command;
		int background;
		int redirectOut;
		int redirectIn;
		int comment;
		int commandArraySize;
		char *commandArray[512];
	};

struct shellSession
	{
		int *lastForegroundPid;
		int *pidArraySize;
		int *pidArray[40];
		int *foreground;
	};

//parses input token and updates argument pointer
struct usrInput *parseInput(char *currLine){

	//create struct allocate space
	struct usrInput *currInput = malloc(sizeof(struct usrInput));

	// Exctract command
		
		//convert $$ to PID
		char expand[2048];
		char newToken[2048] = "";
		char gotPid[2048];
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

		currInput->command = calloc(strlen(token)+1, sizeof(char));
		
 		strcpy(currInput->command, token);
		int i = 0;
		
		
		//Set argument array
		while(1) {

			if (i==0) {
				currInput->commandArray[i] = calloc(strlen(currInput->command)+1, sizeof(char));
				strcpy(currInput->commandArray[i], currInput->command);
				i++;
				continue;
			};
			
			
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
		currInput->comment = 0;
		

		//Check if command is a comment, and set indicator for execution skip later
		for(int i = 0; i < strlen(currInput->command);i++){
		if (currInput->command[i] == '#') {
			
					currInput->comment = 1;
					fflush(stdout);
				};
		}
		//iterate through command line and add to argument list
			for (int j = 0;j < i; j++) {

				if (j > 0) {

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
				
			}
			}

	return currInput;
}

int foreground = 0;

int main(){

	//struct to track overall user session info
	struct shellSession *currSession = malloc(sizeof(struct shellSession));
	currSession->lastForegroundPid = calloc(4, sizeof(int));
	currSession->pidArraySize = calloc(4, sizeof(int));
	currSession->foreground = calloc(4, sizeof(int));
	*currSession->foreground = 0;

	// Handler for SIGNINT
void handle_SIGTSTP(int signo){

	if(foreground == 0){
		char* message = "\nEntering foreground-only mode (& is now ignored)\n:";
		write(STDOUT_FILENO, message, 50);
		foreground = 1;
		clearerr(stdin); // reset stdin status
	}
	else{
		
		char* message = "\nExiting foreground-only mode\n:";
		write(STDOUT_FILENO, message, 30);
		foreground = 0;
		clearerr(stdin); // reset stdin status
		clearerr(stdout);
	};
	
	// Raise SIGUSR2. However, since this signal is blocked until handle_SIGNIT
	// finishes, it will be delivered only when handle_SIGINT finishes
	// Sleep for 10 seconds
	sleep(1);
}

void handle_SIGINT(int signo){
	
	sleep(1);
	clearerr(stdin); // reset stdin status
}

// Handler for SIGUSR2
void handle_SIGUSR2(int signo){
	char* message = "Caught SIGUSR2, exiting!\n";
	write(STDOUT_FILENO, message, 25);
	clearerr(stdin); // reset stdin status
	exit(0);
}

	// Initialize SIGINT_action struct to be empty
	struct sigaction SIGTSTP_action = {{0}}, SIGINT_action = {{0}}, SIGUSR2_action = {{0}}, ignore_action = {{0}};

	/*SIGINT_action*/

		//Fill out the SIGINT_action struct

		// Register handle_SIGINT as the signal handler
		SIGINT_action.sa_handler = handle_SIGINT;
		// Block all catchable signals while handle_SIGINT is running
		sigfillset(&SIGINT_action.sa_mask);
		// No flags set
		SIGINT_action.sa_flags = SA_RESTART;
		sigaction(SIGINT, &SIGINT_action, NULL);
		
	/*handle_SIGSTP */


		//Fill out the SIGINT_action struct
		// Register handle_SIGINT as the signal handler
		SIGTSTP_action.sa_handler = handle_SIGTSTP;
		// Block all catchable signals while handle_SIGINT is running
		sigfillset(&SIGTSTP_action.sa_mask);
		// No flags set
		SIGTSTP_action.sa_flags = SA_RESTART;
		sigaction(SIGTSTP, &SIGTSTP_action, NULL);

	/*handle_SIGUSR2 */

		//Fill out the SIGUSR2_action struct

		// Register handle_SIGUSR2 as the signal handler
		SIGUSR2_action.sa_handler = handle_SIGUSR2;
		// Block all catchable signals while handle_SIGUSR2 is running
		sigfillset(&SIGUSR2_action.sa_mask);
		// No flags set
		SIGUSR2_action.sa_flags = SA_RESTART;
		sigaction(SIGUSR2, &SIGUSR2_action, NULL);
		
	// The ignore_action struct as SIG_IGN as its signal handler
	 ignore_action.sa_handler = SIG_IGN;


	

	//count loops, used for session PID tracking
	int looper = 0;
	

	//Iterate as shell
	while(1) {
		looper++;
		
		//function variables
		int childStatus;
		char scanInput[2048];
		char scanInput1[2048];


		//display prompt..get command
		//ran into an issue with reading a sentence this resource help: 
			// Citation for the following code:
			// Date: 25/10/2021
			// Adapted from: stackoverflow
			// Source URL:https://stackoverflow.com/questions/18547004/how-to-scanf-full-sentence-in-c
		
		printf(": ");
		fflush(stdout);
		fgets(scanInput,2048,stdin);
		if (sscanf(scanInput, "%s", scanInput1) == -1){
			continue;
			}	

		//parse command with parsedInput Struct
		struct usrInput *parsedInput = parseInput(scanInput);	


		/*Built in Commands: ls, cd, exit*/

		//if user enteres exit
		if (strcmp(parsedInput->command, "exit")==0){
				
				//Clean up by freeing parsed input
				free(parsedInput->command);
				for(int j=0;j<parsedInput->commandArraySize;j++) {
					free(parsedInput->commandArray[j]);
				};
				
				free(parsedInput);

				//Clean up by freeing curr session
				for(int j=0;j<*currSession->pidArraySize;j++) {
					free(currSession->pidArray[j]);
				};
				free(currSession->pidArraySize);
				free(currSession->lastForegroundPid);
				free(currSession->foreground);
				free(currSession); 
				
				//kill parent and child processes
				kill(getpid(), SIGTERM);
			};

		//if user enteres cd
		if (strcmp(parsedInput->command, "cd")==0){
			 
			//if user only enters cd withouth arguments
			//take to home dir
			if (parsedInput->commandArraySize == 1) {
				
			 fflush(stdout);
			chdir(getenv("HOME"));
			}

			//if user specifies where to go
			else{
			chdir(parsedInput->commandArray[1]);	
			}
			
			//Clean up by freeing parsed-input
			free(parsedInput->command);
			for(int j=0;j<parsedInput->commandArraySize;j++) {
				free(parsedInput->commandArray[j]);
			};
			
			free(parsedInput);
			continue;
		};

		//if command was status
		if (strcmp(parsedInput->command, "status")==0){
			
			//No child processes yet on first loop so exit with good status
			if (looper == 1) {printf("Exit status %d \n", WEXITSTATUS(0));}

			//Any other loop, will determin if a process exited abnormally or not
			else {if(WIFEXITED(childStatus)){ 
				printf("Exit status %d\n", WEXITSTATUS(childStatus));
				fflush(stdout);
			
			//abnormal exit
			} else{
				printf("Terminated by signal %d\n", WTERMSIG(childStatus));
				fflush(stdout);}}

			//Clean up by freeing parsed input
			free(parsedInput->command);
			for(int j=0;j<parsedInput->commandArraySize;j++) {
				free(parsedInput->commandArray[j]);
			};
			
			free(parsedInput);
			continue;
		};
		

		// Fork a new process
		pid_t spawnPid = fork();

		//Switch between parent and child process
		switch(spawnPid){

		//if fails to fork
		case -1:
			perror("error in the fork\n");
			fflush(stdout);
			exit(1);
			break;
		
		//if fork works, run child process
		case 0: ;
		
			sigaction(SIGTSTP, &ignore_action, NULL);
			sigaction(SIGINT, &ignore_action, NULL);
			//if user didn't redirect stdout
				// Citation for the following code:
				// Date: 28/10/2021
				// Adapted from: stackoverflow
				// Source URL: https://stackoverflow.com/questions/14846768/in-c-how-do-i-redirect-stdout-fileno-to-dev-null-using-dup2-and-then-redirect  		
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

			parsedInput->commandArray[parsedInput->commandArraySize] = NULL;
			//iterate through arguments for stdout operator
			for (int j =0;j < parsedInput->commandArraySize;j++){
				
				//if background operator, remove from command
				if (strcmp(parsedInput->commandArray[j], "&")==0){
					//free before removing
					free(parsedInput->commandArray[j]);
					
					//set next element to current	
					parsedInput->commandArray[j] = parsedInput->commandArray[j+1];
					parsedInput->commandArraySize--;
					continue;
				};

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

					//free before removing
					free(parsedInput->commandArray[j]);
					free(parsedInput->commandArray[j+1]);
					//set next element to current	
					parsedInput->commandArray[j] = parsedInput->commandArray[j+2];
					parsedInput->commandArraySize--;
					parsedInput->commandArraySize--;

					
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

			/*PROCESS REDIRECT IN */

				//FOUND redirect in
				if (strcmp(parsedInput->commandArray[j], "<")==0){

					
					
					//set source
					char* fileName = parsedInput->commandArray[j+1];
					int fdIn = open(fileName, O_RDONLY);

					//file open failed
					if (fdIn == -1){
            			printf("cannot open %s for input\n", fileName);
						fflush(stdout);
            			exit(1);
        			}
					
					//sys redirect
					dup2(fdIn, STDIN_FILENO);
 

					//free before removing
					free(parsedInput->commandArray[j]);
					free(parsedInput->commandArray[j+1]);
					
					//set next element to current	
					parsedInput->commandArray[j] = parsedInput->commandArray[j+2];
					
					parsedInput->commandArraySize--;
					parsedInput->commandArraySize--;
					
				}}

				

			/*PROCESS COMMAND*/


			
			
			parsedInput->commandArray[parsedInput->commandArraySize] = NULL;

			
			if (execvp(parsedInput->command, parsedInput->commandArray) != 0) {
				if (parsedInput->comment > 0) {
					break;
				}
					
					perror("");
					fflush(stdout);
					exit(1);
				};


				//all went well exit normal
				exit(0);

		//parent process
		default:
			
			//check to see if a child processed in th background completed
			for(int j=0;j<*currSession->pidArraySize;j++) {

				//found completed child
				if (waitpid(*currSession->pidArray[j], &childStatus, WNOHANG) != 0 ) {
					
					//exit messages
					if(WIFEXITED(childStatus)){
						printf("Background pid %d is done, exit status %d\n", *currSession->pidArray[j], childStatus);
						fflush(stdout);
					} else{
						printf("Background pid %d is done, terminated by signal %d\n", *currSession->pidArray[j], WTERMSIG(childStatus));
						fflush(stdout);
					}
					

					//delete from array
						// Citation for the following code:
						// Date: 30/10/2021
						// Adapted from : programmingsimplified.com
						// Source URL:https://www.programmingsimplified.com/c/source-code/c-program-delete-element-from-array
					
					//free before removing
					free(currSession->pidArray[j]);

					//set next element to current	
					currSession->pidArray[j] = currSession->pidArray[j+1];
					*currSession->pidArraySize = *currSession->pidArraySize - 1;
				};
			};

		//track most current foreground process for status function 
		if (parsedInput->background == 0) {*currSession->lastForegroundPid = spawnPid;}

		//Handle background processing and tracking
		if ((parsedInput->background > 0) && foreground != 1) {

			//add elements to an array that records any process 
			//that was run in background
			currSession->pidArray[*currSession->pidArraySize] = calloc(4, sizeof(int));
			*currSession->pidArray[*currSession->pidArraySize] = spawnPid;
			*currSession->pidArraySize = *currSession->pidArraySize + 1;
			
			//Will not wait to complete, hence background
			spawnPid = waitpid(spawnPid, &childStatus, WNOHANG);
			printf("background pid is %d\n", *currSession->pidArray[*currSession->pidArraySize - 1]);
			fflush(stdout);
			}
		//Handle foreground processing
		else{
			//sigaction(SIGTSTP, &ignore_action, NULL);
			sigaction(SIGINT, &SIGUSR2_action, NULL);

			//wait for child to complete
			spawnPid = waitpid(spawnPid, &childStatus, 0);}
			
			SIGTSTP_action.sa_handler = handle_SIGTSTP;
		// Block all catchable signals while handle_SIGINT is running
		sigfillset(&SIGTSTP_action.sa_mask);
		// No flags set
		SIGTSTP_action.sa_flags = SA_RESTART;
			sigaction(SIGTSTP, &SIGTSTP_action, NULL);

			//clean up by freeing parsed input
			free(parsedInput->command);
			for(int j=0;j<parsedInput->commandArraySize;j++) {
				free(parsedInput->commandArray[j]);
			};

			free(parsedInput);
			
		}
		continue;
	}

	//clean up by freeing user session struct
	for(int j=0;j<*currSession->pidArraySize;j++) {
		free(currSession->pidArray[j]);
		};
	free(&currSession->pidArraySize);
	free(currSession->lastForegroundPid);
	free(currSession->foreground);
	free(currSession); 
	
	return 0;
}