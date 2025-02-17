#include "singly_linked_list.h"

singly_linked_list_t *sll_init_list() {
    singly_linked_list_t *list = malloc(sizeof(singly_linked_list_t));
    if (!list)
        return NULL;

    list->size = 0;
    list->head = NULL;
    list->tail = NULL;

    return list;
}

void sll_destroy_list(singly_linked_list_t *list) {
    if (!list)
        return;

    if (list->size == 0) {
        free(list);
        return;
    }

    node_t *ptr = list->head;
    node_t *next_ptr;

    while (ptr != NULL) {
        next_ptr = ptr->next;
        if (ptr->value) free(ptr->value);
        free(ptr);
        ptr = next_ptr;
    }

    free(list);
}

void sll_insert_node(singly_linked_list_t *list, void *data) {
    if (!list || !data)
        return;


    node_t *node = malloc(sizeof(node_t));
    node->value = data;
    node->next = NULL;

    if (list->head == NULL && list->tail == NULL) {
        list->head = node;
        list->tail = node;
    }
    else {
        list->tail->next = node;
        list->tail = node;
    }

    (list->size)++;
}

int sll_remove_node(singly_linked_list_t *list, void *data) {
    if (!list || !data)
    {
        return -1;
    }

    int count = 0;
    node_t *cur_ptr = list->head;
    node_t *next_ptr;
    node_t *prev_ptr = NULL;
    node_t *free_ptr = NULL;

    while (cur_ptr != NULL) {
        next_ptr = cur_ptr->next;

        if (cur_ptr->value == data) {
            //found a node to delete

            free_ptr = cur_ptr;

            if (cur_ptr == list->head) {
                //We are still at the head

                cur_ptr = next_ptr;
                list->head = cur_ptr;
            }
            else {
                //We are in the middle of the list

                prev_ptr->next = next_ptr;
                cur_ptr = next_ptr;
            }

            free(free_ptr);
            (list->size)--;
            count++;
        }
        else {
            prev_ptr = cur_ptr;
            cur_ptr = next_ptr;
        }
    }

    if (list->tail == free_ptr) {
        list->tail = prev_ptr;  // Update tail correctly
    }

    return count;
}

void *sll_remove_top(singly_linked_list_t *list) {
    if (!list)
    {
        return NULL;
    }

    if (list->head == NULL) {
        return NULL;
    }

    node_t *cur_ptr = list->head;
    node_t *next_ptr = cur_ptr->next;
    list->head = next_ptr;

    void *value = cur_ptr->value;

    free(cur_ptr);
    (list->size)--;

    return value;
}

void sll_reverse_list(singly_linked_list_t *list) {
    if (!list)
        return;

    node_t *prev_ptr = NULL;
    node_t *cur_ptr = list->head;
    node_t *next_ptr = cur_ptr->next;

    list->tail = list->head;

    while(cur_ptr != NULL) {
        next_ptr = cur_ptr->next;
        cur_ptr->next = prev_ptr;
        prev_ptr = cur_ptr;
        cur_ptr = next_ptr;
    }

    list->head = prev_ptr;
}

int sll_size(singly_linked_list_t *list) {
    if (!list) return -1;
    return (list->size);
}

node_t *sll_front(singly_linked_list_t *list) {
    if (!list) return NULL;
    if (!list->head) return NULL;
    return (list->head);
}

node_t *sll_back(singly_linked_list_t *list) {
    if (!list) return NULL;
    if (!list->tail) return NULL;
    return (list->tail);
}

void sll_pretty_print_list(singly_linked_list_t *list) {
    if (!list) return;

    node_t *ptr = list->head;
    int count = 0;

    while (ptr != NULL) {
        printf("Node %d Value: %p\n", count, ptr->value);
        ptr = ptr->next;
        count++;
    }
}