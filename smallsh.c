//
//  smallsh.c
//  small shell program
//
//  Created by Andrew Brown on 5/2/16.
//  OSU CS344-400
//

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <termios.h>
//for opening files
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

char* getCommand();
char** parseCommand(char* command, int* argsNum);
int execBuiltIn(char** args, int* exitStatus, int* length);
int execForeign(char** args, int* exitStatus, int* length);
int arrContainsString(char** arr, char* string, int* length);
char** trim(char** arr, int* size);
static void killZombies(int signal);

int main(int argc, const char * argv[]) {

    int exitStatus = 0;
    int argsNum = 0;
    int shellPrompt = 0;

    //set up to kill zombies
    struct sigaction act;
    act.sa_handler = killZombies;
    act.sa_flags = 0;
    sigaction(SIGCHLD, &act, NULL);

    //main loop
    do{
        //variables to hold input
        char* args;
        char** parsedInput = NULL;
        int process = 0;

        //get command returns raw input from user
        args = getCommand();

        //parse command returns tokenized args
        parsedInput = parseCommand(args, &argsNum);

        //printf("number of args: %d\n", argsNum);

        //TODO: check for background run command
        //this will tell us if command was built in or not
        //I did it this way because we only need to run the execForeign if the command was not buil in
        int nativeCommand;
        nativeCommand = execBuiltIn(parsedInput, &exitStatus, &argsNum);
        if (nativeCommand == 1) {
            //user chose to exit
            shellPrompt = 1;
        }
        else if (nativeCommand == 0) {
            shellPrompt = 0;
            //restart loop because we know user entered built in command
            continue;
        } else {
            //note: we shouldn't get here if the user entered a built in command
            exitStatus = execForeign(parsedInput, &exitStatus, &argsNum);
            if (exitStatus != 0){
                printf("smallsh: no such file or directory\n");
            }
        }

        //printf("are we there yet: %d\n", shellPrompt);
        //free up memory associated with the input
        free(args);
        free(parsedInput);

    }while(shellPrompt == 0);
}

/* Function: execforeign
 * takes array of args, pointer to status, pointer to length of args array
 * executes non built in commands and returns status
 */
int execForeign(char** args, int* exitStatus, int* length){
    // i/o redirection args
    int outputRedirect = arrContainsString(args, ">", length);
    int inputRedirect = arrContainsString(args, "<", length);
    int bgProcess = arrContainsString(args, "&", length);

    //printf("background process: %d\n", bgProcess);
    //printf("length of array: %d\n", *length);

    int fd = -1; //file descriptor for opening files
    char* file; //will hold filename
    char** trimmed = NULL; //will hold the input array after &, < and > are cut off
    pid_t childPid = -5; //process id for child created in fork
    struct sigaction act;
    int exitMethod, childStatus = 0;

    //check for bgProcess
    //remember arrContainsString returns position in the array
    //so if the user put '&' last, then we need to run process in bg
    //also i'm checking if length is greater than 1, cause if the user entered 1 arg it's pointless
    if (bgProcess == *length-1 && *length > 1) {
        bgProcess = 1; // at this point it's just a 0 or 1 binary
    }

    //check for output redirect
    if (outputRedirect > 0) {
        //add 1 b/c we returned the spot where the > is
        file = args[outputRedirect + 1];
        fd = open(file, O_WRONLY|O_TRUNC|O_CREAT, 0644);
        //printf("opened file: %s\n", args[outputRedirect+1]);
    }
    //check for input redirect
    if (inputRedirect > 0) {
        file = args[inputRedirect + 1];
        fd = open(file, O_RDONLY, 0644);
    }

    //create child process
    childPid = fork();

    switch (childPid) {
        //if fork < 0 fork was not successful
        case -1:
            exit(1);
            break;
        //if fork == 0 child process
        case 0:
            //if user selected output redirect, replace standard output with output file
            //reference: http://www.cs.loyola.edu/~jglenn/702/S2005/Examples/dup2.html
            //exit if unable to create output file
            if (outputRedirect > 0 && (dup2(fd, 1) < 0)) {
                printf("smallsh: unable to open specified file: %s\n", file);
                fflush(stdout);
                _exit(1);
            }
            //if user chose input redirect, attempt open input file
            if (inputRedirect && (dup2(fd, 0) < 0)) {
                printf("smallsh: unable to open specified file: %s\n", file);
                fflush(stdout);
                _exit(1);
            }

            //if we are running a foreground process
            if (bgProcess ==0) {
                //user sig_dfl macro to user default, ie do not ignore signial interrupts
                //this is because we are running in the foreground
                //ref:http://www.gnu.org/software/libc/manual/html_node/Sigaction-Function-Example.html
                act.sa_handler = SIG_DFL;
                act.sa_flags = 0;
                sigaction(SIGINT, &act, NULL);
            }

            //close file descriptor
            close(fd);
            //trim the user args array so that execvp wont try to execute <, > and &
            trimmed = trim(args, length);


            //will execute first argument and takes array of supporting arguments
            //DOES NOT RETURN
            exitMethod = execvp(args[0], trimmed);
            if ( exitMethod == -1) {
                //if we make it here, execvp failed bc the exec family functions do not return
                printf("%s: no such file or directory\n", args[0]);
                //exit with error
                _exit(1);
            } else if (exitMethod == 0) {
                printf("not expecting return. Must be an execvp() error. \n");
                _exit(1);
            }
                break;
        default: // if fork > 0, this part is executed by parent process
            close(fd);

            //if we are running a foreground command
            if (bgProcess == 0) {
                //we want the parent process to ignore signals for now
                //cause if user hits ctrl + c right now, it should kill child not parent
                act.sa_handler = SIG_DFL;
                act.sa_handler = SIG_IGN;
                sigaction(SIGINT, &act, NULL);

                int result;
                //wait for child process to finish up
                result = waitpid(childPid, &childStatus, WUNTRACED);
                if (result == -1) {
                    perror ("waitpid");
                    exit(1);
                }

                //set exit status
                exitMethod = WEXITSTATUS(childStatus);
            }


            // if (WIFSIGNALED(status)) {
            //     int sig = WTERMSIG(status);
            //     char errMsg[50];
            //     char si
            // }

            break;
    }//end of case switch

    //code here executed by both

    //if running foreground
    if (bgProcess == 0) {
        //important to restore this because if user hits ctrl + c again, they want to exit the shell
        act.sa_handler = SIG_DFL;
        sigaction(SIGINT, &act, NULL);
    }
    free(trimmed);
    return exitMethod;
}


/*Function: killZombies
 * takes in int which is the signal
 * used for when child process is running in background
 * ref: http://voyager.deanza.edu/~perry/sigchld.html
 */
static void killZombies(int signal){
    int status;
	pid_t childPid;

	//Loop will continue as long as there are children to reap
    //http://stackoverflow.com/questions/11322488/how-to-make-sure-that-waitpid-1-stat-wnohang-collect-all-children-process
	while ((childPid = waitpid(-1, &status, WNOHANG)) > 0)
	{
		//copy the childPid to a string so we can concatenate to an output msg
		char childPidStr[10];
		snprintf(childPidStr, sizeof(childPidStr), "%d", childPid);
        //use same steps to create a message for finished bg processes
		char bgMessage[80];
		strncpy(bgMessage, "\nbackground process ", 80);
		strcat(bgMessage, childPidStr);
		strcat(bgMessage, " finished, ");

		//detect interupt signals
		if(WIFSIGNALED(status)) {
			int signalNumber = WTERMSIG(status);
			char signalNumberStr[10];
   			snprintf(signalNumberStr, sizeof(signalNumberStr), "%d", signalNumber);
			strcat(bgMessage, "terminated by signal: ");
			strcat(bgMessage, signalNumberStr);
			strcat(bgMessage, "\n");
            //use write instead of printf: http://stackoverflow.com/questions/14647468/about-fork-and-printf-write
			write(1, bgMessage, sizeof(bgMessage));
		}
        //if the process ended normally
		else
		{
			char statusNumberStr[5];
			strcat(bgMessage, "exit value: ");
			snprintf(statusNumberStr, sizeof(statusNumberStr), "%d", WEXITSTATUS(status));
			strcat(bgMessage, statusNumberStr);
			strcat(bgMessage, "\n");
			write(1, bgMessage, sizeof(bgMessage));
		}
		continue;
	}
}

/* Function: execBuiltIn
 * takes ptr to ptr of array containing parsed input from user
 * returns an int which will be used to determine if loop continues
 */
int execBuiltIn(char** args, int* exitStatus, int* length){
    //if user entered a comment
    if(strcmp(args[0], "#") == 0){
        	return 0;
    }
    //returns a 0 so the loop in main will stop
    else if(strcmp(args[0], "exit") == 0){
        	return 1;
    }

    else if(strcmp(args[0], "status") == 0){
            printf("exit status: %d\n", *exitStatus);
        	return 0;
    }

    else if(strcmp(args[0], "cd") == 0){
        char** trimmed = NULL;
        trimmed = trim(args, length);
        if(trimmed[1] == NULL){
    	        	char* path = getenv("HOME");
    	        	chdir(path);
    	   	} else {
       			char* path = args[1];
	    		chdir(path);
       		}
            //free(trimmed);
        	return 0;
    }
    //if we made it this far, none of the build in commands were typed
    //returning -1 signifies to main that it was not a built in command
    return -1;
}

/* Function: getCommand()
 * takes no parameters, is called from main
 * returns ptr to an array which will contain raw input from user
 */
char* getCommand(){
    //stdin flush
    tcflush(0, TCIFLUSH);

    char* input = malloc(sizeof(char*) * 2048);
    int arrSize = 2048;

    fflush(stdout);
    printf(": ");
    fgets(input, arrSize, stdin);
    fflush(stdout);

    return input;
}

/* Function: parseCommand
 * takes ptr to array containing raw input from user
 * returns ptr to parsed input (weeds out " " and \n, returns array commands)
 */
char** parseCommand(char* command, int* argsNum){
    char** tokens = malloc(sizeof(char*) * 512);
    char* arg = NULL;

    //separate the string from the newline character
    arg = strtok(command, "\n");

    //if no commands were entered, return
    if(arg == NULL){
		tokens[0] = NULL;
		return tokens;
	}

    int i = 0;
    //start getting each block of chars delimited by spaces from the input command
    arg = strtok(arg, " ");
    //as long as there is still stuff to parse
	while (arg != NULL){
		tokens[i] = arg;
		i++;
		arg = strtok(NULL, " ");
	}
    //store the number of arguments we got.
    *argsNum = i;
    return tokens;
}

/* Function: arrContainsString
 * takes ann array to search, and a string to search for
 * returns 0 or 1 to specify if match was found
 * reference: http://stackoverflow.com/questions/13677890/how-to-check-if-a-string-is-in-an-array-of-strings-in-c
 */
int arrContainsString(char** arr, char* string, int* length){
    int i;
    //loop over array and compare to string
    for(i = 0; i < *length; i++){
        if(strcmp(arr[i], string)==0){
            //return the position in the array so we can find the file by adding 1
            return i;
        }
    }
    //if we made it this far, no match
    //we can return 0 here because the < or > wouldn't be the first arg
    return 0;
}

/* Function: trim
 * takes an array and an int for length of that array
 *  cuts off array where so built in commands are not fed into the foreign exec
 */
char** trim(char** arr, int* size){
    char** trimmed = malloc(sizeof(char*) * 512);
    int i = 0;
    //we're just going to loop over the array until we hit anything we don't want execvp handling
    while (i < *size) {
        if ((strcmp(arr[i], "<") != 0) && (strcmp(arr[i], ">") != 0) && strcmp(arr[i], "&") != 0) {
            trimmed[i] = arr[i];
            i++;
            continue;
        }
        //outside of loop we have hit one of those characters, so stop the array hence trimming it
        break;
    }
    return trimmed;
}
