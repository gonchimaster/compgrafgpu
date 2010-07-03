#include <stdlib.h>
#include <stdio.h>
#include "ListaEnteros.h"

ListaEnteros ListaEnterosCreate(){
	return NULL;
}

bool ListaEnterosIsEmpty(ListaEnteros lista){
	return lista == NULL;
}

ListaEnteros ListaEnterosInsert(ListaEnteros lista, int elemento){
	NodoListaEnteros* nuevoNodo = (NodoListaEnteros*)malloc(sizeof(NodoListaEnteros));
	nuevoNodo->elemento = elemento;
	nuevoNodo->siguente = lista;
	return nuevoNodo;
}

int ListaEnterosHead(ListaEnteros lista){
	if(lista != NULL){
		return lista->elemento;
	}
	return NULL;
}

ListaEnteros ListaEnterosTail(ListaEnteros lista){
	if(lista != NULL){
		return lista->siguente;
	}
	return NULL;
}

void ListaEnterosPrint(ListaEnteros lista){
	while(lista != NULL){
		printf("%d ", lista->elemento);
		lista = lista->siguente;
	}
	printf("\n");
}

void ListaEnterosDispose(ListaEnteros lista){
	while(lista != NULL){
		NodoLista* aux = lista;
		lista = lista->siguente;
		free(aux);
	}
}