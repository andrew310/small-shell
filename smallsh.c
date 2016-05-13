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
//for opening files
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

char* getCommand();
char** parseCommand(char* command, int* argsNum);
int execBuiltIn(char** args, int* exitStatus);
int execForeign(char** args, int* exitStatus, int* length);
int arrContainsString(char** arr, char* string, int* length);

int main(int argc, const char * argv[]) {

    int exitStatus = 0;
    int argsNum = 0;

    int shellPrompt = 1;
    do{
        char* args;
        char** parsedInput = NULL;
        int process = 0;
        args = getCommand();
        printf(args); //for testing
        printf("hi from main\n"); //for testing

        parsedInput = parseCommand(args, &argsNum);

        printf("number of args: %d\n", argsNum);

        //TODO: check for background run command
        int nativeCommand;
        nativeCommand = execBuiltIn(parsedInput, &exitStatus);
        if (nativeCommand == 0) {
            //user chose to exit
            shellPrompt = 0;
        }
        else if (nativeCommand == 1) {
            shellPrompt = 1;
            //restart loop because we know user entered built in command
            continue;
        } else {
            //note: we shouldn't get here if the user entered a built in command
            shellPrompt = execForeign(parsedInput, &exitStatus, &argsNum);
        }

        //free up memory associated with the input
        free(args);
        free(parsedInput);

    }while(shellPrompt == 1);
}

int execForeign(char** args, int* exitStatus, int* length){
    // i/o redirection args
    int outputRedirect = arrContainsString(args, ">", length);
    int inputRedirect = arrContainsString(args, "<", length);
    int fd;
    char* file;

    printf("search array results: %d\n", outputRedirect);

    if (outputRedirect) {
        //add 1 cause we returned the spot in the array where the > was
        file = args[outputRedirect + 1];

    }
    if (inputRedirect) {
        file = args[inputRedirect + 1];
    }
    printf("length of ur array here we go: %d\n", *length);
    int i;
    //loop over array and compare to string
    for(i = 0; i < *length; i++){
        printf(args[i]);
    }



    // else if(((strcmp(args[1], ">") == 0) || (strcmp(args[1], "<") == 0))){
    //     //make temporary copy of file descriptors for stdin and stdout
    //     //reference: http://stackoverflow.com/questions/4832603/how-could-i-temporary-redirect-stdout-to-a-file-in-a-c-program
    //     int bak, new;
    //     fflush(stdout);
    //     bak = dup(1);
    //     //3rd argument should contain the filename
    //     new = open(args[2], O_WRONLY|O_CREAT|O_TRUNC, 0664);
    //     //error handling for opening file
    //     if(new == -1) {
    //       printf("smallsh: no such file or directory");
    //       fflush(stdout);
    //       *exitStatus = 1;
    //     }
    //
    //     dup2(new, 1);
    //     close(new);
    //     //code here...
    //     fflush(stdout);
    //     dup2(bak, 1);
    //     close(bak);
    // }

    printf("hello from execforeign\n");
    return 1;
}

/* Function: execBuiltIn
 * takes ptr to ptr of array containing parsed input from user
 * returns an int which will be used to determine if loop continues
 */
int execBuiltIn(char** args, int* exitStatus){
    printf("hello from built in\n");
    //if user entered a comment
    if(strcmp(args[0], "#") == 0){
        	return 1;
    }
    //returns a 0 so the loop in main will stop
    else if(strcmp(args[0], "exit") == 0){
        	return 0;
    }

    else if(strcmp(args[0], "status") == 0){
            printf("exit status: %d\n", *exitStatus);
        	return 1;
    }

    else if(strcmp(args[0], "cd") == 0){
        if(args[1] == NULL){
    	        	char* path = getenv("HOME");
    	        	chdir(path);
    	   	} else {
       			char* path = args[1];
	    		chdir(path);
       		}
        	return 1;
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
    char* input = malloc(sizeof(char*) * 2048);
    int arrSize = 2048;

    printf(": ");
    fgets(input, arrSize, stdin);

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
    printf("size of your array:%d\n", *length);


    //loop over array and compare to string
    for(i = 0; i < *length; i++){
        printf("heres your stuff %s\n", arr[i]);
        if(strcmp(arr[i], string)==0){
            //return the position in the array so we can find the file by adding 1
            return i;
        }
    }
    //if we made it this far, no match
    //we can return 0 here because the < or > wouldn't be the first arg
    return 0;
}
