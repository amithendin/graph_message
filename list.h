//
// Created by amit on 8/13/23.
//

#ifndef DISTMSG_LIST_H
#define DISTMSG_LIST_H

#include <stdlib.h>

typedef struct listnode {
    void *value;
    struct listnode *next;
    struct listnode *prev;
} ListNode;

typedef struct list {
    unsigned int size;
    struct listnode *top;
    struct listnode *bottom;
} List;

List *new_list();

void free_list(List *s);

void list_push(List *s, void *value);

void *list_pop(List *s);

void *list_poplast(List *s);

void *list_top(List *s);

void *list_bottom(List *s);

char *consume_buff_chain(List *buff_chain, long total_size);

#endif //DISTMSG_LIST_H
