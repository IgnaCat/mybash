#include <stdlib.h>
#include <stdbool.h>

#include "parsing.h"
#include "parser.h"
#include "command.h"
#include "execute.h"
#include "builtin.h"
#include "string.h"

#include <assert.h>
#include <unistd.h> //para syscalls
#include <errno.h>  //errors

#include <fcntl.h>
#include <sys/wait.h>

#include <sys/sendfile.h> 

#include <sys/stat.h>
#include "tests/syscall_mock.h"

#ifdef DUPLICATE_OUTPUT
static void duplicate_output(scommand cmd,unsigned int cmd_number, unsigned int pipe_length, int **fd){
//leer el archivo(especificado en cmd) y copiarlo en el pipe correspondiente: fd[cmd_number][1];
    if(cmd_number != pipe_length-1){
        
        int read_fd = open(scommand_get_redir_out(cmd), O_RDONLY); //abro un fd apuntando al archivo a leer.

        struct stat *buf = malloc(sizeof(struct stat)); 

        if(fstat(read_fd, buf)==-1){
            perror("Error on getting file stats");
        }
        
        if(sendfile(fd[cmd_number][1], read_fd, NULL,buf->st_size) == -1){
            perror("Error on copying file to pipe");
        }
        free(buf);
        close(read_fd);
    }
    
}
#endif

static void create_pipes(unsigned int pipe_length, int **fd)
{ 
    for (unsigned int i = 0; i < pipe_length - 1; ++i)
    {
        if (pipe(fd[i]) < 0)
        {
            printf("Error on creating pipe\n");
            exit(1);
        }
    }      
}

static void destroy_output_pipes(unsigned int pipe_length, int **fd)
{ 
    for (unsigned int i = 0; i < pipe_length - 1; ++i)
    {
        if (close(fd[i][0]) < 0)
        {
            printf("Error on destroying pipe\n");
            exit(1);
        }
        //free(fd[i]); libero ?
        
    }      
}

static void execute_external_command(scommand cmd)
{   
    unsigned int cmd_length = scommand_length(cmd);
    char ** myargs = malloc(cmd_length + 1 * sizeof(char *));   //arreglo que guardara el nombre y los argumentos del comando a correr, esta memoria no se libera por free ya que se corre un execvp
    unsigned int j = 0;
    while (!scommand_is_empty(cmd))
    {
        myargs[j] = strdup(scommand_front(cmd));
        scommand_pop_front(cmd);
        ++j;
    }
    assert(j == cmd_length);
    myargs[cmd_length] = NULL;  //indica la finalizacion de los argumentos
    
    if (execvp(myargs[0], myargs) < 1)
    {
        printf("Error on execute\n");
        exit(1);
    }
    //free(*myargs); free a myargs ?
}

static void change_data_redirection(scommand cmd, unsigned int cmd_number,unsigned int pipe_length, int **fd)
{
    //parte de la redireccion de salida:
    {

        if (cmd_number < pipe_length - 1  &&  scommand_get_redir_out(cmd) == NULL)   //esto se deberia hacer si el comando no especifica salida puntual y no es el ultimo
        {
            if (dup2(fd[cmd_number][1], STDOUT_FILENO) < 0) //redireccion del pipe de stdout del hijo al pipe correspondiente de salida
            { 
                perror("Error on cloning fd\n");
            }
        }

        if (scommand_get_redir_out(cmd) != NULL)
        { // Si el comando tiene especificado una salida, la abrimos y no se escribe en el pipe de salida de este comando
            int out_fd = open(scommand_get_redir_out(cmd), O_CREAT | O_WRONLY | O_TRUNC, S_IRWXU); // Le asigno un file descriptor a la entrada
            if(dup2(out_fd, STDOUT_FILENO)<0)
            {//cambio el fd del stdout por el nuevo fd del nuevo archivo
                perror("Error on cloning fd\n");
            } 
        }
    }
    //parte de la redireccion de entrada:
    {
        if (cmd_number > 0  &&  scommand_get_redir_in(cmd) == NULL)
        {                                     // Si el proceso no es el primero y no tiene entrada especificada, entonces redirecciono el pipe que conecta con el proceso anterior al stdin.
            if(dup2(fd[cmd_number - 1][0], STDIN_FILENO) < 0){// redireccion del pipe del stdin del hijo al pipe correspondiente de entrada
                perror("Error on cloning fd\n");
            }
        }

        if (scommand_get_redir_in(cmd) != NULL)
        { // Si el comando tiene especificado una entrada, la abrimos y no se consume lo del pipe de entrada de este comando
            int in_fd = open(scommand_get_redir_in(cmd), O_RDONLY, S_IRUSR); // Le asigno un file descriptor a la entrada
            if(dup2(in_fd, STDIN_FILENO)<0)
            { // cambio el fd de stdin por el fd del archivo redireccionado al comando. ESto hace que cuando haga el execpv tome este input.
                perror("Error en cloning fd\n");
            }
        }
    }
}

static int running_command(scommand cmd, unsigned int cmd_number, unsigned int pipe_length, int **fd){
    int rc = fork();
    if(rc < 0){
        //error
        printf("Error while creating a fork\n");
        exit(1);
    }
    if(rc > 0){
        //el padre
#ifdef DUPLICATE_OUTPUT
        if(dup_flag && scommand_get_redir_out(cmd) != NULL){
            waitpid(rc, NULL, 0);
            duplicate_output(cmd, cmd_number, pipe_length,  fd);
        }
        //aca se podria agregar el punto estrella de la doble redireccion de salida
        //si tuvo especificada una salida distinta a la de defecto
        //copiamos el archivo en el pipe de entrada que le correspondia a este proces
#endif

        //se van cerrando los input de los pipes para que se indique que se termino de escribir al proceso que sigue en el pipeline.
        if(pipe_length>1 && cmd_number < pipe_length - 1){
            close(fd[cmd_number][1]);
        }
        return rc;

        
    }else{
        //el hijo
        change_data_redirection(cmd, cmd_number, pipe_length, fd);
        
        if(builtin_is_internal(cmd)){
            builtin_run(cmd);
        }else{ 
            execute_external_command(cmd);
        }
    }   
    return 0;
}

static void command_admin(pipeline apipe){
    unsigned int pipe_length = pipeline_length(apipe);
    // si tengo procesos 1,2,3,4,5 -> 1 comunica con 2, 2 con 3, 3 con 4 y 4 con 5. en general para N procesos -> 2*(N-1) file descriptors.
    // fd[][0] es la punta de lectura y fd[][1] es la punta de escritura.
    int **fd = calloc(2, sizeof(int*));
    fd[0] = calloc(pipe_length-1, sizeof(int));
    fd[1] = calloc(pipe_length-1, sizeof(int));


    create_pipes(pipe_length, fd);
    int * pids = calloc(pipe_length, sizeof(int));
    for(unsigned cmd_number = 0; cmd_number < pipe_length; ++cmd_number){
        pids[cmd_number] = running_command(pipeline_front(apipe), cmd_number, pipe_length, fd);
        pipeline_pop_front(apipe);
    }
    for(unsigned cmd_number = 0; cmd_number < pipe_length; ++cmd_number){
        waitpid(pids[cmd_number], NULL, 0);
        }
    destroy_output_pipes(pipe_length, fd);
    free(fd[0]);
    free(fd[1]);
    free(fd);
    fd = NULL;
}

//esta funcion llama a el manager para ejecutar un pipeline, y lo espera o no segun se quiera ejecutar en background o no
void execute_pipeline(pipeline apipe){
    assert(apipe != NULL);
    //lo primero que vamos a ver es si es un comando interno y es unico en ese caso corremos el buitin_run
    if(builtin_alone(apipe) 
        && strcmp(scommand_front(pipeline_front(apipe)), "echo") 
        && strcmp(scommand_front(pipeline_front(apipe)), "help"))
        {
        builtin_run(pipeline_front(apipe));
    }
    else if((apipe != NULL) && (pipeline_length(apipe) > 0)) {  //caso que sean mas de 1 comando o uno externo 
        int rc = fork();
        if(rc < 0){
            //error
            printf("Error while forking, process_admin\n");
            exit(1);
        }
        if(rc > 0){
            //the parent
            if(pipeline_get_wait(apipe) == true){
                waitpid(rc, NULL, 0);
            }
        }else{
            //the child, command manager
            command_admin(apipe);
            exit(0); //Salgo del primer proceso hijo que cree, si no lo hago, el proceso hijo sale al prompt, que es desde donde se llamo esta funcion inicialmente; En dicho caso por cada comando que llame
                     //se empezarian a acumular los padres en espera, y requeriria  1 exit por padre para salir del prompt.
        }
    }
}
