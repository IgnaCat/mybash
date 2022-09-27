#include "builtin.h"
#include "command.h"
#include "execute.h"

#include <assert.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include "tests/syscall_mock.h"

#define LEN_IC 5
#define MAX_TYPE_CHAR 100
const char * internal_commands[] = {
    "cd",
    "help",
    "exit",
    "echo",
    "duplicate",

};



bool builtin_is_internal(scommand cmd){ //las ideas para esta funcion fueron sacadas del apunte del teorico en el apartado de process api. Y se investigo sobre el comando type con su flag -t para que responda con una sola palabra si es un comando interno o no
    int p[2];
    if (pipe(p) < 0){
        exit(1);
    }
    int rc = fork();
    if(rc<0){
        perror("Error on fork while checking if command is internal");
        exit(1);
    }
    if(rc == 0){ //hello im a child!
        close(STDOUT_FILENO);
        dup2(p[1], STDOUT_FILENO);
        char * shell_cmd;
        shell_cmd = malloc((strlen(scommand_front(cmd))+ 18) * sizeof(char));
        shell_cmd[0] = '\0';
        shell_cmd = strcat(shell_cmd, "bash -c 'type -t ");
        shell_cmd = strcat(shell_cmd, scommand_front(cmd));
        shell_cmd = strcat(shell_cmd,"'");
        system(shell_cmd);

        close(p[0]);
        close(p[1]);
        free(shell_cmd);
        exit(EXIT_SUCCESS);
        
    }
    else{   //hello THE parent!
        waitpid(rc, NULL, 0);   
            close(p[1]);
            char *data = malloc(MAX_TYPE_CHAR * sizeof(char));
            int i = 0;
            read(p[0], data, MAX_TYPE_CHAR);
            close(p[0]);
            if(strcmp(data,"builtin")==10){
                //el comando es builtin
                free(data);
                return true;
            }else if(strcmp(data,"file")==10)
            {
                //el comando es file
                free(data);
                return false;
            }
            else{
                //error! el comando no es ni builtin ni file
                //printf("%s: command not found\n", scommand_front(cmd));
                for(int i = 0; i < LEN_IC; i++) {
                    if(strcmp(internal_commands[i], scommand_front(cmd)) == 0) {
                    free(data);
                    return true;
                    }
                }
                free(data);
                return false;
            }
        }
        return true;
    } 


bool builtin_alone(pipeline p){
return ((p != NULL) && (pipeline_length(p) == 1) && builtin_is_internal(pipeline_front(p)));  //creo que asi se cortocicuitaba un and, con un solo operador
}

void builtin_run(scommand cmd){
    assert(builtin_is_internal(cmd));
    char * command = scommand_front(cmd);
    unsigned int i = 0;
    bool implemented = false;
    while (i < LEN_IC)
    {
        if (!strcmp(internal_commands[i],command))
        {
            switch (i)
            {
            case 0:
                scommand_pop_front(cmd);
                command = scommand_to_string(cmd);
                int size = strlen(command); 
                command[size-1] = '\0';
                if (chdir(command) != 0) {
                    perror("failed");
                }
                break;
            case 1:
                printf("\n>>mybash - Authors: Alejo Corral, Nahuel Fredes, Francisco Martiarena, Ignacio Martinez\nInternal Commands:\n  cd <changes directory>\n  help <info>\n  exit <ends mybash>\n  echo <mirror>\n  duplicate <activate or desactivate output duplication> recieves true or false\n");
                break;
            case 2:
                scommand_destroy(cmd);  //finalizar limpiamente
                exit(0);
                break;
            case 3: //echo
                scommand_pop_front(cmd);
                printf("%s\n", scommand_front(cmd));
                break;
            case 4: //duplicate
                scommand_pop_front(cmd);
                if(!scommand_is_empty(cmd) && !strcmp(scommand_front(cmd),"true")){
                    printf("the duplication is activated\n");
                    dup_flag = true;
                }else{
                    printf("the duplication is deactivated\n");
                    dup_flag = false;
                }
                
                break;
            default:
                printf("Error, no builtin implemented");
                //si un comando es internal no debeeria llegar aca ya que estaria si o si en la lista implementada
                break;
            }
            implemented = true;
        }
        ++i;
    }
    if (!implemented)
    {
        printf("Error, no builtin implemented\n");
    }
    
}