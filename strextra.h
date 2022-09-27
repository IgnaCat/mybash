#ifndef _STREXTRA_H_
#define _STREXTRA_H_


char * strmerge(char *s1, char *s2);    //perguntar si la podemos eliminar, ya que usamos strcat
/*
 * Concatena las cadenas en s1 y s2 devolviendo nueva memoria (debe ser
 * liberada por el llamador con free())
 *
 * USAGE:
 *
 * merge = strmerge(s1, s2);
 *
 * REQUIRES:
 *     s1 != NULL &&  s2 != NULL
 *
 * ENSURES:
 *     merge != NULL && strlen(merge) == strlen(s1) + strlen(s2)
 *
 */


#endif
