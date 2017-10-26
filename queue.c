//------------------------------------------//
// Aluno: Douglas Zaia Adam                 //
// GRR: 20157412        Matéria: S.O.       //
//------------------------------------------//

#include <stdio.h>
#include <stdlib.h>
#include "queue.h"

void queue_append (queue_t **queue, queue_t *elem) {
	queue_t *aux = (*queue);

	// verifica se a fila e o elemento existem
	if(elem == NULL) {
		printf("Elemento a ser inserido não existe\n");
		return;
	}
    if((*queue) == NULL) {
		(*queue) = elem;
		(*queue)->prev = elem;
		(*queue)->next = elem;
		return;
	}

	// verifica se o elemento pertence a outra fila analisando se o prev e next são NULL, caso sejam ele não está em outra fila
	if(elem->prev != NULL || elem->next != NULL) {
		printf("O elemento a ser inserido já pertence a uma fila (obs: podendo ser essa mesma tambem)\n");
		return;
	}
	aux = (*queue)->prev;
	aux->next = elem;
	elem->prev = aux;
	elem->next = (*queue);
	(*queue)->prev = elem;
}

queue_t *queue_remove (queue_t **queue, queue_t *elem) {
	queue_t *aux = (*queue);
	queue_t *aux2;

	if((*queue) == NULL) {
		printf("Fila nao existe\n");
		return NULL;
	}
	if(elem == NULL) {
		printf("Elemento a ser removido não existe\n");
		return NULL;
	}
   	// remove caso exista na fila e só tenha um elemento na fila
	if((*queue)->prev == (*queue) && (*queue) == elem) {
        (*queue) = NULL;
        elem->prev = NULL;
        elem->next =NULL;
        return elem;
	}

    	// for o primeiro elemento
	if(elem == (*queue)) {
        aux = (*queue)->prev;
        (*queue) = (*queue)->next;
        aux->next = (*queue);
        (*queue)->prev = aux;
        elem->prev = NULL;
		elem->next = NULL;
        return elem;
	}
    	// verifica se existe o elem na fila
	while(aux != elem) {
		aux = aux->next;
		if(aux == (*queue)) {
			printf("Elemento nao existe na fila\n");
			return NULL;
		}
	}
	// se for outro elemento
	if(aux == elem) {
		aux2 = aux->next;
		aux = aux->prev;
		aux2->prev = aux;
		aux->next = aux2;
		elem->prev = NULL;
		elem->next = NULL;
		return elem;
	}
	return NULL ;
}

int queue_size (queue_t *queue) {
	int count = 1;
	queue_t *aux;

	if(queue == NULL) {
		return 0; 
	}

	aux = queue;
	while(aux->next != queue) {
		aux = aux->next;
		count++;
	}
	return count;
}

void queue_print (char *name, queue_t *queue, void print_elem (void*) ) {
    printf("%s ", name);
    queue_t *aux = queue;
    printf("[");
    if(queue != NULL) {
        do {
            print_elem(aux);
            printf(" ");
            aux = aux->next;
        } while(aux != queue);
    }
    printf("]\n");
}