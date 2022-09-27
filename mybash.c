#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <unistd.h>
#include <sys/wait.h>
#include "command.h"
#include "execute.h"
#include "parser.h"
#include "parsing.h"
#include "builtin.h"

#define RED "\033[0;31m"
#define MAG "\033[0;35m"

static void reset(void) {
  printf("\033[0m");
}

static void show_prompt(void) {
    char cwd[1024];
    getcwd(cwd, 1024);
    printf(RED);
    printf ("mybash>:");
    printf(MAG);
    printf ("%s$ ",cwd);
    reset();
    fflush (stdout); 
}

int main(int argc, char *argv[]) {
    pipeline pipe;
    Parser input;
    bool quit = false;

    input = parser_new(stdin);
    while (!quit) {
        show_prompt();
        pipe = parse_pipeline(input); 
        quit = parser_at_eof(input);    
        if(pipe!=NULL){
            execute_pipeline(pipe); 
        }
        pipe = pipeline_destroy(pipe);
        while(waitpid(-1, NULL, WNOHANG) > 0);
    }
    parser_destroy(input);
    input = NULL;
    return EXIT_SUCCESS;
}

