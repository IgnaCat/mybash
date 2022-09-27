#include <stdlib.h>
#include <stdbool.h>

#include "parsing.h"
#include "parser.h"
#include "command.h"

static scommand parse_scommand(Parser p) {  //esta funcion debe ser robusta, ya que tiene contacto con lo que ingtresa el usuario
    arg_kind_t type;
    scommand cmd = scommand_new();
    char * data;
    parser_skip_blanks(p);  //sacamos los tabs y espacios del comienzo
    data = parser_next_argument(p, &type);    //hay que ver si lo primero que se recibe deberia ser el nombre del comando o puede ser flexible a la reordenacion
    if(type != ARG_NORMAL || data == NULL){
        return NULL;
    }
    scommand_push_back(cmd, data);
    //ahora leemos los demas argumenteos, entrada y salida del comando
    while(true){
        parser_skip_blanks(p);  //quitamos todos los espacios en blanco


        data = parser_next_argument(p, &type);
        if(data == NULL){   //habria que ver si funciona asi pero podriamos haber llegado al final de la lectura del command
            return cmd;
        }else{
            switch(type){
                case ARG_NORMAL:
                scommand_push_back(cmd,data);
                break;
                case ARG_INPUT:
                scommand_set_redir_in(cmd, data);
                break;
                case ARG_OUTPUT:
                scommand_set_redir_out(cmd, data);
                break;
                default:
                //si se llega a este punto es porque ocurrio un error pues parse_next_argument no deberia retornar algo distinto de lo enumerado
                return NULL;
            }
            //free(data); //liberamos este string ya que se hace una copia en el scommand
        }
    }
    /* Devuelve NULL cuando hay un error de parseo */
    return NULL;
}

pipeline parse_pipeline(Parser p) {
    pipeline result = pipeline_new();
    scommand cmd = NULL;
    bool error = false, another_pipe=true;

    cmd = parse_scommand(p);    //tener en cuenta que esta funcion consume el pipe
    error = (cmd==NULL); /* Comando inválido al empezar */
    //si ocurre un error al comienzo debemos hacer clean up y retornar NULL
    if(error == true){
        bool garbage;
        parser_garbage(p, &garbage);
        return NULL;
    }
    while (another_pipe && !error) {
        pipeline_push_back(result, cmd);    //agregamos el comando a la pipeline
        parser_skip_blanks(p);
        parser_op_pipe(p, &another_pipe);   //checkeamos si hay que agregar otro comando en la pipeline
        if(another_pipe == true){
            cmd = parse_scommand(p);
            if(cmd == NULL){
                //este caso seria si luego de un pipe no se pudo leer el comando
            }
        }
    }
        parser_skip_blanks(p);
        bool background_op = false;
        parser_op_background(p, &background_op);
        pipeline_set_wait(result, !background_op);
        bool garbage;
        parser_garbage(p, &garbage);
        if(garbage == true){
            return NULL;    //no deberia haber basura luego de terminar la lectura del pipeline
        }


    return result; // MODIFICAR
}
