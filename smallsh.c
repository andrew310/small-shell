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
int execBuiltIn(char** args, int* exitStatus);
int execForeign(char** args, int* exitStatus, int* length);
int arrContainsString(char** arr, char* string, int* length);
char** trim(char** arr, int* size);

int main(int argc, const char * argv[]) {

    int exitStatus = 0;
    int argsNum = 0;

    int shellPrompt = 0;
    do{
        char* args;
        char** parsedInput = NULL;
        int process = 0;

        args = getCommand();


        parsedInput = parseCommand(args, &argsNum);

        printf("number of args: %d\n", argsNum);

        //TODO: check for background run command
        int nativeCommand;
        nativeCommand = execBuiltIn(parsedInput, &exitStatus);
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
        }

        printf("are we there yet: %d\n", shellPrompt);
        //free up memory associated with the input
        free(args);
        free(parsedInput);

    }while(shellPrompt == 0);
}

int execForeign(char** args, int* exitStatus, int* length){
    // i/o redirection args
    int outputRedirect = arrContainsString(args, ">", length);
    int inputRedirect = arrContainsString(args, "<", length);
    int fd = -1;
    char* file;
    char** trimmed = NULL;
    pid_t childPid = -5;
    struct sigaction act;
    int exitMethod, childStatus = 0;

    //printf("search array results: %d\n", outputRedirect);

    if (outputRedirect > 0) {
        //add 1 b/c we returned the spot where the > is
        file = args[outputRedirect + 1];
        fd = open(file, O_WRONLY|O_TRUNC|O_CREAT, 0644);
        //printf("opened file: %s\n", args[outputRedirect+1]);
    }
    if (inputRedirect > 0) {
        file = args[inputRedirect + 1];
        fd = open(file, O_RDONLY, 0644);
    }

    //printf("input redirect: %d\n", inputRedirect);

    // printf("length of ur array here we go: %d\n", *length);
    // int i;
    // //loop over array and compare to string
    // for(i = 0; i < *length; i++){
    //     printf(args[i]);
    // }

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
                _exit(1);
            }

            if (inputRedirect && (dup2(fd, 0) < 0)) {
                printf("smallsh: unable to open specified file: %s\n", file);
                _exit(1);
            }

            //user sig_dfl macro to user default, ie do not ignore signial interrupts
            //this is because we are running in the foreground
            //ref:http://www.gnu.org/software/libc/manual/html_node/Sigaction-Function-Example.html
            act.sa_handler = SIG_DFL;
            sigaction(SIGINT, &act, NULL);

            close(fd);

            trimmed = trim(args, length);


            //will execute first argument and takes array of supporting arguments
            //DOES NOT RETURN
            if (execvp(args[0], trimmed) == -1) {
                //if we make it here, execvp failed bc the exec family functions do not return
                printf("%s: no such file or directory\n", args[0]);
                //exit with error
                _exit(1);
            }
                break;
        default: // if fork > 0, this part is executed by parent process
            close(fd);

            //we want the parent process to ignore signals
            act.sa_handler = SIG_DFL;
            act.sa_handler = SIG_IGN;
            sigaction(SIGINT, &act, NULL);

            //wait for child process to finish up
            waitpid(childPid, &childStatus, 0);

            exitMethod = WEXITSTATUS(childStatus);

            // if (WIFSIGNALED(status)) {
            //     int sig = WTERMSIG(status);
            //     char errMsg[50];
            //     char si
            // }

            break;
    }//end of case switch

    //code here executed by both
    return exitMethod;
}

/* Function: execBuiltIn
 * takes ptr to ptr of array containing parsed input from user
 * returns an int which will be used to determine if loop continues
 */
int execBuiltIn(char** args, int* exitStatus){
    printf("hello from built in\n");
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
        if(args[1] == NULL){
    	        	char* path = getenv("HOME");
    	        	chdir(path);
    	   	} else {
       			char* path = args[1];
	    		chdir(path);
       		}
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
    while (i < *size) {
        if ((strcmp(arr[i], "<") != 0) && (strcmp(arr[i], ">") != 0) && strcmp(arr[i], "&") != 0) {
            trimmed[i] = arr[i];
            printf("I BE TRIMMIN: %s\n", trimmed[i]);
            i++;
            continue;
        }
        //outside of loop we have hit one of those characters, so stop the array hence trimming it
        break;
    }
    return trimmed;
}
