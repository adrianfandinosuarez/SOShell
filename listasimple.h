#ifndef LISTASIMPLE_H
#define LISTASIMPLE_H


#include <stdlib.h>
#include <stdio.h>
#include <errno.h>

/*la lista en un simple array de punteros terminado a NULL*/
/*sirve para el PATH, la lista de hostoricos, la lista de ficheros abiertos..*/

#define MAXLISTASIMPLE 4096
typedef void * LISTASIMPLE [MAXLISTASIMPLE];

int AniadirElemento (LISTASIMPLE l, void *el);
void ImprimirListaFirst (LISTASIMPLE l, int cuantos, int n,void (*ImprimirElemento)(void*)); 
void ImprimirListaLast (LISTASIMPLE l, int cuantos, int n,void (*ImprimirElemento)(void*));
void ImprimirListaCompleta (LISTASIMPLE l, int n,void (*ImprimirElemento)(void*));
void BorrarLista(LISTASIMPLE l);

void * GetPrimerElemento(LISTASIMPLE l);
void *GetSiguienteElemento(LISTASIMPLE l);
void* GetSiguienteElementoAdd(LISTASIMPLE l, void * el,int (*comp)(void *,void*));
void* GetElementoAtPos (LISTASIMPLE l, int n);
int BorrarElementoAtPos (LISTASIMPLE l, int n);
int BuscarElemento (LISTASIMPLE l, void * el, int (*comp)(void*,void*));
int BorrarElemento (LISTASIMPLE l, void * el,int (*comp)(void*,void*));


#endif
