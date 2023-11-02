/**
 * Author: Amit Hendin
 * Date: 8/13/2023
 *
 * Implementation of table.h
 */
#include "table.h"
#include "list.h"
#include "util.h"

#include <stdlib.h>
#include <string.h>

Table *new_table() {
    int i;
    Table *t;
    t = malloc(sizeof(Table)); /* allocate space for the table */
    t->size = 0;

    for (i = 0; i < TABLE_SIZE; i++) {/* initialize all pointers in the table to NULL */
        t->arr[i] = NULL;
    }

    return t;
}

void free_table(Table *t) {
    int i;
    TableNode *n, *tmp;
    for (i = 0; i < TABLE_SIZE; i++) { /* Iterate over all cells in the array */
        n = t->arr[i];
        while (n != NULL) { /* Iterate over the entire chain in the cell and free all nodes*/
            tmp = n->next;
            free(n->key.data);
            free(n);
            n = tmp;
        }
    }
    free(t);/* free table */
}

int table_hash(Table *mp, Buffer key) {
    int bucketIndex; /* index calculated */
    int c;
    char *key_data;
    unsigned long hash;

    hash = START_HASH;
    key_data = (char*)key.data;

    while ((c = (int)(*key_data++) )) { /* iterate over all bytes in the buffer */
        hash = ((hash << HASH_SHIFT) + hash) + c; /* hash * 33 + c */
    }
    bucketIndex = hash % TABLE_SIZE; /* make sure hash in within limits of table index*/

    return bucketIndex;
}

void table_insert(Table *t, Buffer key, Buffer value) {
    int bucketIndex;
    TableNode *newNode;
    /* Allocate space for a new table node, copy the key buffer and set the pointer of the value to the value pointer provided*/
    newNode = malloc(sizeof(TableNode));
    newNode->key.len = key.len;
    newNode->key.data = malloc(key.len);
    memcpy(newNode->key.data, key.data, key.len);
    newNode->value = value;
    bucketIndex = table_hash(t, key);
    newNode->next = NULL;

    if (t->arr[bucketIndex] == NULL) { /* if cell is free set it to new node */
        t->arr[bucketIndex] = newNode;

    } else if (buffer_cmp(newNode->key, t->arr[bucketIndex]->key) !=0) { /* else push new node to the top of the chain */
        newNode->next = t->arr[bucketIndex];
        t->arr[bucketIndex] = newNode;
    }
    t->size ++; /* keep count of the number of key/value pairs in the table */
}

void table_delete(Table *t, Buffer key) {
    int bucketIndex;
    TableNode *prevNode, *currNode;

    prevNode = NULL;
    bucketIndex = table_hash(t, key);/* calculate hash from key */
    currNode = t->arr[bucketIndex];

    while (currNode != NULL) { /* if the cell in the table is occupied, keep looping until the node in the cell's chain has the same key as the input key or until the end of the chain */
        if (buffer_cmp(key, currNode->key) == 0) { /* if we find a node in the chain that has the same key, delete it from the table be either disconneting it from the chain
 * or setting the cell in the array to NULL */
            if (currNode == t->arr[bucketIndex]) { /* If node is head of list, replace head of list with the next node*/
                t->arr[bucketIndex] = currNode->next;

            } else {
                prevNode->next = currNode->next; /*otherwise, connect the previouse node to the next node*/
            }
            /* decrease node count and free memory */
            t->size --;
            free(currNode->key.data);
            free(currNode);
            break;
        }
        /* keep track of previous node and iterate through all nodes */
        prevNode = currNode;
        currNode = currNode->next;
    }
}

Buffer *table_search(Table *t, Buffer key) {
    int bucketIndex;
    TableNode *bucketHead;

    bucketIndex = table_hash(t, key); /* calculate hash from key */
    bucketHead = t->arr[bucketIndex];

    while (bucketHead != NULL) { /* if cell node empty, iterate through chain at cell to check if a node in the chain has identical key*/
        if (buffer_cmp(bucketHead->key, key) == 0) {
            return &bucketHead->value; /* if a node with an identical key is found, return it's value */
        }

        bucketHead = bucketHead->next;
    }
    /* otherwise, return  null */
    return NULL;
}

TableIter* table_iter_new(Table *mp) {
    TableIter *it;
    it = malloc(sizeof(TableIter)); /*allocate memory for array index, table and node pointers */
    it->i = -1; /* set to -1 because upon first call of table_iter_next we increase index by 1 at the beginning of the function */
    it->curr = NULL;
    it->mp = mp;
    return it;
}

int table_iter_next(TableIter *it) {
    if (it->curr == NULL) { /* If new iterator or reached end of cell */
        it->i++;/* increase index counter */
        while (it->i < TABLE_SIZE && it->mp->arr[it->i] == NULL) it->i++; /* if next cell is empty, keep iterating through cells until a non empty one is found or we reach end of array */
        if (it->i >= TABLE_SIZE) { /* if we reach end of array return NULL since it is also the end of the table */
            return 0;
        }
        it->curr = it->mp->arr[it->i]; /* otherwise, set iterator node pointer to the first node in the chain at cell i */

        if (it->curr == NULL) {
            return 0;
        }

    }else {
        /* otherwise, we are iterating through a chain therefore we simple move the node pointer to the next one in the chain */
        it->curr = it->curr->next;
        if (it->curr == NULL) {
            return table_iter_next(it);
        }
    }
    /* If not returned 0 then we have iterated so we return 1 */
    return 1;
}

Buffer *serialize_table(Table *mp) {
    TableIter *it;
    Buffer *tmp;
    Buffer *total_buff;
    List* buff_chain;
    size_t key_len, val_len;

    total_buff = (Buffer *)malloc(sizeof (Buffer));

    /* iterate over all key/value pairs in the table */
    it = table_iter_new(mp);
    total_buff->len = 0;
    buff_chain = new_list();
    while (table_iter_next(it)) {
        key_len = it->curr->key.len;
        val_len = it->curr->value.len;
        /* for each pair, encode the length of the key, then the key data, then length of the value, then the value data in sequence in a buffer */
        tmp = (Buffer *)malloc(sizeof (Buffer));
        tmp->len = key_len + sizeof (size_t) + val_len + sizeof (size_t);
        tmp->data = (char *)malloc(sizeof(char) * tmp->len );
        memset(tmp->data, 0, tmp->len);

        memcpy(tmp->data, &key_len, sizeof (size_t));
        memcpy(tmp->data + sizeof (size_t), it->curr->key.data, key_len);
        memcpy(tmp->data + sizeof (size_t) + key_len, &val_len, sizeof (size_t));
        memcpy(tmp->data + sizeof (size_t) + key_len + sizeof (size_t), it->curr->value.data, val_len);
        /* add said buffer to a list of buffers*/
        list_push(buff_chain, tmp);
        total_buff->len += tmp->len;
    }
    /* concat list of buffers into a single large buffer*/
    total_buff->data = consume_buff_chain(buff_chain, total_buff->len);
    /* free iterator and return large buffer */
    free(it);
    it = NULL;

    return total_buff;
}

DeserializeTableIter *deserialize_table_iter(Buffer *table_buff) {
    DeserializeTableIter *it;
    it = malloc(sizeof(TableIter));/* Allocate space for char pointer to hold the current position in the buffer and the address of the last element in the buffer */
    it->curr_pos = table_buff->data;
    it->end_pos = table_buff->data +table_buff->len; /* address of last element in the buffer so we know to stop when curr_pos >= end_pos */
    it->curr = NULL;
    return it;
}

int deserialize_table_iter_next(DeserializeTableIter *it) {
    if (it->curr_pos >= it->end_pos) { /* If we reached/passed the end of the buffer, return 0 to indicate no more elements in the buffer*/
        return 0;
    }
    if (it->curr == NULL) {/* If current node pointer is null, create new empty node */
        it->curr = malloc(sizeof(TableNode));
        it->curr->key.len = 0;
        it->curr->key.data = NULL;
        it->curr->value.len = 0;
        it->curr->value.data = NULL;
        it->curr->next = NULL;
    }
    /* if we have data from previouse node, free it*/
    if (it->curr->key.data != NULL) {
        free(it->curr->key.data);
    }
    if (it->curr->value.data != NULL) {
        free(it->curr->value.data);
    }

    /* Copy key length from node, from it we know how many bytes to take for the key, copy the key*/
    memcpy(&it->curr->key.len, it->curr_pos, sizeof (size_t));
    it->curr->key.data = malloc(sizeof (char) * it->curr->key.len);
    memcpy(it->curr->key.data, it->curr_pos + sizeof (size_t), it->curr->key.len);
    /* After the key ends there is the length of the value, copy the length of the value so we know where the value ends and copy the value*/
    memcpy(&it->curr->value.len, it->curr_pos + sizeof (size_t) + it->curr->key.len, sizeof (size_t));
    it->curr->value.data = malloc(sizeof (char) * it->curr->value.len);
    memcpy(it->curr->value.data, it->curr_pos  + sizeof (size_t) + it->curr->key.len + sizeof (size_t), it->curr->value.len);
    /* increase the current position in the buffer by however many bytes we read */
    it->curr_pos += sizeof (size_t) + it->curr->key.len + sizeof (size_t) + it->curr->value.len;
    /* If we didn't return 0 then we read a key/value pair so return 1 */
    return 1;
}

