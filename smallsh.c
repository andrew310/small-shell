//
//  smallsh.c
//  small shell program
//
//  Created by Andrew Brown on 5/2/16.
//  OSU CS344-400
//

#include <stdio.h>

char** getCommand(int process);

int main(int argc, const char * argv[]) {

    char** args;
    int process = 0;
    
    args = getCommand(process);
    
    printf(args);
    
    return 0;
}

char** getCommand(int process){
    //string
    char** args = malloc(sizeof(char*) * 512);
    char* input = malloc(sizeof(char*) * 2048);
    int arrSize = 2048;
    
    printf(": ");
    
    fgets(input, arrSize, stdin);
    
    input = strtok(input, "\n");
    
    if (input == NULL) {
        args[0] = NULL;
        return args;
    }
    
    
    return input;
}
