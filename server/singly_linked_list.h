#ifndef SINGLY_LINKED_LIST_H
#define SINGLY_LINKED_LIST_H

#include <stdlib.h>
#include <stdio.h>

typedef struct node {
    void *value;
    struct node *next;
} node_t;

typedef struct singly_linked_list {
    int size;
    node_t *head;
    node_t *tail;
} singly_linked_list_t;

singly_linked_list_t *sll_init_list();
void sll_destroy_list(singly_linked_list_t *list);
void sll_insert_node(singly_linked_list_t *list, void *data);
int sll_remove_node(singly_linked_list_t *list, void *data);
void *sll_remove_top(singly_linked_list_t *list);
void sll_reverse_list(singly_linked_list_t *list);
int sll_size(singly_linked_list_t *list);
void *sll_front(singly_linked_list_t *list);
void *sll_back(singly_linked_list_t *list);
void sll_pretty_print_list(singly_linked_list_t *list);

#endif