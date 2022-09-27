#include <glib-2.0/glib.h>
#include <assert.h>
#include "command.h"
#include <string.h>

//Tad scommand:

struct scommand_s {
    GSList * arg_name; //GSList es una estructura que nos da glib representando una lista simple, tiene un puntero a un dato y un puntero al siguiente de la lista. Aca guardo los argumentos del comando simple.
    char* arg_input;
    char* arg_output;
};

scommand scommand_new(void){
    scommand self = NULL;
    self = malloc(sizeof(struct scommand_s));
    self->arg_name = NULL;
    self->arg_input = NULL;
    self->arg_output = NULL;
    assert(self != NULL && scommand_is_empty(self) &&
           scommand_get_redir_in(self) == NULL
           && scommand_get_redir_out(self) == NULL
          );

    return self;
}

scommand scommand_destroy(scommand self){
    assert(self != NULL);
    while(!scommand_is_empty(self)){
        scommand_pop_front(self);
    }
    g_slist_free(self->arg_name);
    free(self->arg_input);
    free(self->arg_output);
    self->arg_name = NULL;
    self->arg_input = NULL;
    self->arg_output = NULL;
    free(self);
    self = NULL;
    assert(self == NULL);
    return self;
}

void scommand_push_back(scommand self, char * argument){
    assert(self!=NULL && argument!=NULL);
    self->arg_name = g_slist_append(self->arg_name, strdup(argument));
    assert(!scommand_is_empty(self));
    free(argument);
}

void scommand_pop_front(scommand self){
    assert(self!=NULL && !scommand_is_empty(self));
    free(g_slist_nth_data(self->arg_name, 0));
    self->arg_name = g_slist_remove(self->arg_name, g_slist_nth_data(self->arg_name, 0));
}

void scommand_set_redir_in(scommand self, char * filename){
    assert(self!=NULL);
    if(self->arg_input != NULL){
        free(self->arg_input);
    }
    if(filename != NULL){
      self->arg_input = strdup(filename);
    }else{
      self->arg_input = NULL;
    }
    free(filename); //liberamos el string filename ya que hicimos una copia
}

void scommand_set_redir_out(scommand self, char * filename){
    assert(self!=NULL);
    if(self->arg_output != NULL){
        free(self->arg_output);
    }
    if(filename != NULL){
      self->arg_output = strdup(filename);
    }else{
      self->arg_output = NULL;
    }
    free(filename); //liberamos el string filename ya que hicimos una copia
}

bool scommand_is_empty(const scommand self){
    assert(self!=NULL);
    return self->arg_name == NULL;
}

unsigned int scommand_length(const scommand self){
    assert(self!=NULL);
    unsigned int length = g_slist_length(self->arg_name);
    assert((length==0) == scommand_is_empty(self));
    return length;
}

char * scommand_front(const scommand self){
    assert(self!=NULL && !scommand_is_empty(self));
    return g_slist_nth_data(self->arg_name, 0);
}

char * scommand_get_redir_in(const scommand self){
    assert(self!=NULL);
    return self->arg_input;
}

char * scommand_get_redir_out(const scommand self){
    assert(self!=NULL);
    return self->arg_output;
}


char * scommand_to_string(const scommand self){
    assert(self!=NULL);
    char *result;
    result = malloc(sizeof(char));
    result[0] = '\0';
    unsigned int str_len = 1; //por el /0 al final del string
    for(unsigned int i = 0 ; i< scommand_length(self) ; ++i){   //escribimos la lista de comandos en result
        str_len += strlen(g_slist_nth_data(self->arg_name, i))+1;//+1 por el espacio
        result = realloc(result, str_len*sizeof(char));
        assert(result != NULL);
        result = strcat(result, g_slist_nth_data(self->arg_name, i));
        result = strcat(result, " ");
    }

    if (self->arg_input != NULL){
      //escribimos cual es la entrada del scommand
      str_len += strlen(self->arg_input)+3;   //se agrega "< " + " ", en total 3 caracteres extras
      result = realloc(result, str_len*sizeof(char));
      assert(result != NULL);
      result = strcat(result, "< ");
      result = strcat(result, self->arg_input);
      result = strcat(result, " ");
    }

    if (self->arg_output != NULL){
      //ahora lo mismo pero para la salida
      str_len += strlen(self->arg_output)+2;   //se agrega "> ", en total 2 caracteres extras
      result = realloc(result, str_len*sizeof(char));
      assert(result != NULL);
      result = strcat(result, "> ");
      result = strcat(result, self->arg_output);
    }


    assert(scommand_is_empty(self) ||
           scommand_get_redir_in(self)==NULL ||
           scommand_get_redir_out(self)==NULL || strlen(result)>0
          );

    return result;
}

//Tad pipeline:

struct pipeline_s {
    GSList * scommands_l; //lista con los scommand
    bool wait_child;
};

pipeline pipeline_new(void){
    pipeline result = malloc(sizeof(struct pipeline_s));
    result->scommands_l = NULL;
    result->wait_child = true;

    assert(result != NULL && pipeline_is_empty(result) &&
           pipeline_get_wait(result)
          );

    return result;
}


pipeline pipeline_destroy(pipeline self){
    assert(self != NULL);
    while(pipeline_length(self) > 0){
        pipeline_pop_front(self);
    }
    g_slist_free(self->scommands_l);    //faltaria eliminar a lo que apunta cada puntero
    free(self);
    self = NULL;
    assert(self == NULL);
    return self;
}

void pipeline_push_back(pipeline self, scommand sc){
    assert(self!=NULL && sc!=NULL);
    self->scommands_l = g_slist_append(self->scommands_l, sc);
    assert(!pipeline_is_empty(self));
}


void pipeline_pop_front(pipeline self){
    assert(self!=NULL && !pipeline_is_empty(self));
    scommand_destroy(g_slist_nth_data(self->scommands_l, 0));
    self->scommands_l = g_slist_remove(self->scommands_l, g_slist_nth_data(self->scommands_l, 0));
}


void pipeline_set_wait(pipeline self, const bool w){
    assert(self!=NULL);
    self->wait_child = w;
}

bool pipeline_is_empty(const pipeline self){
    assert(self!=NULL);
    return self->scommands_l == NULL;
}


unsigned int pipeline_length(const pipeline self){
    assert(self!=NULL);
    unsigned int length = g_slist_length(self->scommands_l);

    assert((length==0) == pipeline_is_empty(self));
    return length;
}


scommand pipeline_front(const pipeline self){
    assert(self!=NULL && !pipeline_is_empty(self));
    assert(g_slist_nth_data(self->scommands_l, 0) != NULL);
    return g_slist_nth_data(self->scommands_l, 0);
}


bool pipeline_get_wait(const pipeline self){
    assert(self != NULL);
    return self->wait_child;
}


char * pipeline_to_string(const pipeline self){
    assert(self!=NULL);
    char *result = NULL;
    uint result_size = 1;
    result = malloc(result_size*sizeof(char));
    result[0] = '\0';
    for(unsigned int i=0; i<g_slist_length(self->scommands_l); ++i){
        char * simple_command_printed = scommand_to_string(g_slist_nth_data(self->scommands_l, i));  //tratamos a cada uno de los elementos del pipe como scommand y luego vamos a imprimirlos con '|' uniendolos
        result_size += strlen(simple_command_printed);
        result = realloc(result, result_size * sizeof(char));
        result = strcat(result, simple_command_printed);
        free(simple_command_printed);
        if(i+1 < g_slist_length(self->scommands_l)){
            result_size += 3;
            result = realloc(result, result_size * sizeof(char));
            result = strcat(result, " | ");
        }else{
            //aca se puede agregar lo que queremos al final de poner todos los scommands del pipe
        }
    }
    if(!self->wait_child){
        result_size += 2;
        result = realloc(result, result_size * sizeof(char));
        result = strcat(result, " &");
    }

    assert(pipeline_is_empty(self) || pipeline_get_wait(self) || strlen(result)>0);
    return result;
}
