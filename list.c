/**
 * Author: Amit Hendin
 * Date: 8/13/2023
 *
 * Implementation of list.h
 */
#include <string.h>
#include "list.h"
#include "util.h"

List *new_list() {
    List *s;
    s = malloc(sizeof (List)); /* Allocate space for top and bottom pointers and value count, set them to zero */
    s->size = 0;
    s->top = NULL;
    s->bottom = NULL;
}

void free_list(List *s) {
    ListNode *c, *tmp;
    c = s->top;
    while (c != NULL) { /* iterate over all nodes in the list */
        tmp = c->next; /* get pointer to next node */
        free(c); /* free current node */
        c = tmp;/* move on to the next node */
    }
    free(s);/* free list struct */
}

void list_push(List *s, void *value) {
    ListNode *n;
    n = malloc(sizeof(ListNode)); /* Allocate space for new node */
    n->value = value; /* Set node value pointer to given value pointer */
    n->next = s->top; /* set next neightbor of node to the current top node */
    n->prev = NULL; /* set previous neightbor to NULL since this node will now become the top node */
    if (s->top != NULL) { /* If list already has a top node, set it's previouse neightbor to this node */
        s->top->prev = n;
    }
    s->top = n; /* set top node pointer to new node pointer */
    if (s->bottom == NULL) { /* if list doesn't have a bottom node, set it to this one*/
        s->bottom = s->top;
    }
    s->size++; /* keep track of number of nodes inserted */
}

void *list_pop(List *s) {
    ListNode *tmp;

    if (s->top == NULL) { /* if no top node is present then the list is empty, return NULL */
        return NULL;
    }
    /* Take the top node, set the list top node to the next neighbor of the current top node, decrease size and return node */
    tmp = s->top;
    if (s->top->next == NULL) { /* make to to set bottom to NULL if top node is also the last node */
        s->bottom = NULL;
    }
    s->top = s->top->next;
    s->size --;
    return tmp->value;
}

void *list_top(List *s) {
    if (s->top != NULL)
        return s->top->value; /* return value of top node */
    return NULL;
}

void *list_poplast(List *s) {
    ListNode *tmp;

    if (s->bottom == NULL) { /* if not bottom node the list is empty, return NULL */
        return NULL;
    }
    /* Same procedure as with the top node just with the bottom */
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
        return s->bottom->value; /* return value of the bottom node */
    return NULL;
}

char *consume_buff_chain(List *buff_chain, long total_size) {
    int i;
    char *buff;
    ListNode *curr, *tmp;
    Buffer *currbuff;

    buff = malloc(total_size); /* ALlocate space for the total size of the concatinated buffer */
    curr = buff_chain->top;
    i = 0;
    while (curr != NULL) { /* iterate over all values in the list, for each value, copy it to the buffer at the offset
 * location i and increase the offset location by the length of the buffer*/
        tmp = curr->next;
        currbuff = (Buffer*) curr->value;
        memcpy(buff+i, currbuff->data, currbuff->len);
        i += currbuff->len;

        free(currbuff->data); /* free value once it has been copied */
        free(curr); /* free list node */

        curr = tmp;
    }

    free(buff_chain); /* free list */
    return buff; /* return the resulting buffer */
}