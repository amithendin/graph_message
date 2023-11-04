/**
 * Author: Amit Hendin
 * Date: 13/8/2023
 *
 * A standard and simple implementation of a bidirectional linked list data structure with some extra features
 */
#ifndef DISTMSG_LIST_H
#define DISTMSG_LIST_H

#include <stdlib.h>

/**
 * Holds a single node in a list
 */
typedef struct listnode {
    void *value;
    struct listnode *next;
    struct listnode *prev;
} ListNode;
/**
 * Holds the entire list
 */
typedef struct list {
    unsigned int size;
    struct listnode *top;
    struct listnode *bottom;
} List;

/**
 * Creates a new empty list
 *
 * @return A pointer to the new list
 */
List *new_list();
/**
 * Frees a given list and all data it points to except for the values
 *
 * @param s Pointer to the list to free
 */
void free_list(List *s);
/**
 * Pushes value to the top of the list
 *
 * @param s Pointer to list to push value into
 * @param value Point of value to push into list
 */
void list_push(List *s, void *value);
/**
 * Removes a value from the top of the list and returns it
 *
 * @param s Pointer to the list to remove from
 * @return Pointer to the value removed
 */
void *list_pop(List *s);
/**
 * Removes a value from the bottom of the list and returns it
 *
 * @param s Pointer to list to remove from
 * @return Pointer to the value removed
 */
void *list_poplast(List *s);
/**
 * Returns pointer to the value at the top of the list
 *
 * @param s Pointer to list
 * @return Pointer to value at top of the list
 */
void *list_top(List *s);
/**
 * Returns pointer to the value at the bottom of the list
 *
 * @param s Pointer to list
 * @return Pointer to value at the bottom of the list
 */
void *list_bottom(List *s);
/**
 * Takes in a list of Buffer structs and the sum of lengths of those buffers and returns a new buffer which is a concatenation of the buffers in the list
 * while freeing each buffer as it get concatinated to the result
 *
 * @param buff_chain List of buffers
 * @param total_size Sum of lengths of all the buffers
 * @return Buffer which is a concatenation fo all the buffers in the list
 */
char *consume_buff_chain(List *buff_chain, long total_size);

#endif //DISTMSG_LIST_H
