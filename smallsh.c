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

char* getCommand();
char** parseCommand(char* command);
int execBuiltIn(char** args);


int main(int argc, const char * argv[]) {

    int shellPrompt = 1;
    do{
        char* args;
        char** parsedInput = NULL;
        int process = 0;
        args = getCommand();
        printf(args); //for testing
        printf("hi from main"); //for testing

        parsedInput = parseCommand(args);

    //    //print our args for testing, can be deleted later
    //    int i;
    //    int arrSize = sizeof(parsedInput);
    //    for(i = 0; i <= arrSize; i++){
    //        printf(parsedInput[i]);
    //        
    //    }

        //TODO: check for background run command
        shellPrompt = execBuiltIn(parsedInput);
    }while(shellPrompt == 1);

    
}

/* Function: execBuiltIn
 * takes ptr to ptr of array containing parsed input from user
 * returns an int which will be used to determine if loop continues
 */
int execBuiltIn(char** args){
    printf("hello from built in");
    //if user entered a comment
    if(strcmp(args[0], "#") == 0){
        	return 1;
    }
    
    if(strcmp(args[0], "exit") == 0){
        	return 0;
    }
    
    if(strcmp(args[0], "cd") == 0){
        if(args[1] == NULL)		{
    	        	char* path = getenv("HOME");
    	        	chdir(path);
    	   	} else
       		{
       			char* path = args[1];
	    		chdir(path);
       		}
        	return 1;
    }
    return 1;
}

/* Function: getCommand()
 * takes no parameters, is called from main
 * returns ptr to an array which will contain raw input from user
 */
char* getCommand(){
    char* input = malloc(sizeof(char*) * 2048);
    int arrSize = 2048;

    fgets(input, arrSize, stdin);
    
    return input;
}

/* Function: parseCommand
 * takes ptr to array containing raw input from user
 * returns ptr to parsed input (weeds out " " and \n, returns array commands)
 */
char** parseCommand(char* command){
    char** tokens = malloc(sizeof(char*) * 512);
    char* arg = NULL;
    
    //separate the string from the newline character
    arg = strtok(command, "\n");
    
    //if no commands were entered, return
    if(arg == NULL)
	{
		tokens[0] = NULL;
		return tokens;
	}
    
    int i = 0;
    //start getting each block of chars delimited by spaces from the input command
    arg = strtok(arg, " ");
    //as long as there is still stuff to parse
	while (arg != NULL) 
	{
		tokens[i] = arg;
		i++;
		arg = strtok(NULL, " ");
	}
    
    return tokens;
}
