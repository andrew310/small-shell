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

char* getCommand();
char** parseCommand(char* command);

int main(int argc, const char * argv[]) {

    char* args;
    char** parsedInput = NULL;
    char* huh = NULL;
    int process = 0;
    args = getCommand();
    printf(args); //for testing
    printf("hi from main"); //for testing
    
    parsedInput = parseCommand(args);
    
    huh = *parsedInput;
    int i;
    int arrSize = sizeof(parsedInput);
    for(i = 0; i <= arrSize; i++){
        printf(parsedInput[i]);
        
    }
    
    
}

char* getCommand(){
    char* input = malloc(sizeof(char*) * 2048);
    int arrSize = 2048;

    fgets(input, arrSize, stdin);
    
    return input;
}

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
