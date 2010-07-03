#ifndef __LISTAENTEROS__H__
#define __LISTAENTEROS__H__


typedef struct NodoLista {
	int elemento;
	NodoLista* siguente;
} NodoListaEnteros;

typedef NodoListaEnteros* ListaEnteros;


ListaEnteros ListaEnterosCreate();

bool ListaEnterosIsEmpty(ListaEnteros lista);

ListaEnteros ListaEnterosInsert(ListaEnteros lista, int elemento);

int ListaEnterosHead(ListaEnteros lista);

ListaEnteros ListaEnterosTail(ListaEnteros lista);

void ListaEnterosPrint(ListaEnteros lista);

void ListaEnterosDispose(ListaEnteros lista);


#endif