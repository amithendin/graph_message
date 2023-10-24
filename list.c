//
// Created by amit on 8/13/23.
//
#include <string.h>
#include "list.h"
#include "util.h"

List *new_list() {
    List *s;
    s = malloc(sizeof (List));
    s->size = 0;
    s->top = NULL;
    s->bottom = NULL;
}

void free_list(List *s) {
    ListNode *c, *tmp;
    c = s->top;
    while (c != NULL) {
        tmp = c->next;
        free(c);
        c = tmp;
    }
    free(s);
}

void list_push(List *s, void *value) {
    ListNode *n;
    n = malloc(sizeof(ListNode));
    n->value = value;
    n->next = s->top;
    n->prev = NULL;
    if (s->top != NULL) {
        s->top->prev = n;
    }
    s->top = n;
    if (s->bottom == NULL) {
        s->bottom = s->top;
    }
    s->size++;
}

void *list_pop(List *s) {
    ListNode *tmp;

    if (s->top == NULL) {
        return NULL;
    }

    tmp = s->top;
    if (s->top->next == NULL) {
        s->bottom = NULL;
    }
    s->top = s->top->next;
    s->size --;
    return tmp->value;
}

void *list_top(List *s) {
    if (s->top != NULL)
        return s->top->value;
    return NULL;
}

void *list_poplast(List *s) {
    ListNode *tmp;

    if (s->bottom == NULL) {
        return NULL;
    }

    tmp = s->bottom;
    if (s->bottom->prev == NULL) {
        s->top = NULL;
    }
    s->bottom = s->bottom->prev;
    s->size --;
    return tmp->value;
}

void *list_bottom(List *s) {
    if (s->bottom != NULL)
        return s->bottom->value;
    return NULL;
}

char *consume_buff_chain(List *buff_chain, long total_size) {
    int i;
    char *buff;
    ListNode *curr, *tmp;
    Buffer *currbuff;

    buff = malloc(total_size);
    curr = buff_chain->top;
    i = 0;
    while (curr != NULL) {
        tmp = curr->next;
        currbuff = (Buffer*) curr->value;
        memcpy(buff+i, currbuff->data, currbuff->len);
        i += currbuff->len;

        free(currbuff->data);
        free(curr);

        curr = tmp;
    }

    free(buff_chain);
    return buff;
}