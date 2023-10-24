//
// Created by amit on 8/13/23.
//
#include "table.h"
#include "list.h"
#include "util.h"

#include <stdlib.h>
#include <string.h>

Table *new_table() {
    int i;
    Table *t;
    t = malloc(sizeof(Table));
    t->size = 0;

    for (i = 0; i < TABLE_SIZE; i++) {
        t->arr[i] = NULL;
    }

    return t;
}

void free_table(Table *t) {
    int i;
    for (i = 0; i < TABLE_SIZE; i++) {
        if (t->arr[0] != NULL) {
            free(t->arr[0]);
        }
    }
    free(t);
}

int table_hash(Table *mp, Buffer key) {
    int bucketIndex;
    int c;
    char *key_data;
    unsigned long hash;

    hash = START_HASH;
    key_data = (char*)key.data;

    while ((c = (int)(*key_data++) )) {
        hash = ((hash << HASH_SHIFT) + hash) + c; // hash * 33 + c
    }
    bucketIndex = hash % TABLE_SIZE;

    return bucketIndex;
}

void table_insert(Table *t, Buffer key, Buffer value) {
    int bucketIndex;
    TableNode *newNode;

    newNode = malloc(sizeof(TableNode));
    newNode->key.len = key.len;
    newNode->key.data = malloc(key.len);
    memcpy(newNode->key.data, key.data, key.len);
    newNode->value = value;
    bucketIndex = table_hash(t, key);
    newNode->next = NULL;

    if (t->arr[bucketIndex] == NULL) {
        t->arr[bucketIndex] = newNode;

    } else if (buffer_cmp(newNode->key, t->arr[bucketIndex]->key) !=0) {
        newNode->next = t->arr[bucketIndex];
        t->arr[bucketIndex] = newNode;
    }
    t->size ++;
}

void table_delete(Table *t, Buffer key) {
    int bucketIndex;
    TableNode *prevNode, *currNode;

    prevNode = NULL;
    bucketIndex = table_hash(t, key);
    currNode = t->arr[bucketIndex];

    while (currNode != NULL) {
        if (buffer_cmp(key, currNode->key) == 0) {
            if (currNode == t->arr[bucketIndex]) {
                t->arr[bucketIndex] = currNode->next;

            } else {
                prevNode->next = currNode->next;
            }

            t->size --;
            free(currNode->key.data);
            free(currNode);
            break;
        }

        prevNode = currNode;
        currNode = currNode->next;
    }
}

Buffer *table_search(Table *t, Buffer key) {
    int bucketIndex;
    TableNode *bucketHead;

    bucketIndex = table_hash(t, key);
    bucketHead = t->arr[bucketIndex];

    while (bucketHead != NULL) {
        if (buffer_cmp(bucketHead->key, key) == 0) {
            return &bucketHead->value;
        }

        bucketHead = bucketHead->next;
    }

    return NULL;
}

TableIter* table_iter_new(Table *mp) {
    TableIter *it;
    it = malloc(sizeof(TableIter));
    it->i = -1;
    it->curr = NULL;
    it->mp = mp;
    return it;
}

int table_iter_next(TableIter *it) {
    if (it->curr == NULL) {
        it->i++;
        while (it->i < TABLE_SIZE && it->mp->arr[it->i] == NULL) it->i++;
        if (it->i >= TABLE_SIZE) {
            return 0;
        }
        it->curr = it->mp->arr[it->i];

        if (it->curr == NULL) {
            return 0;
        }

    }else {

        it->curr = it->curr->next;
        if (it->curr == NULL) {
            return table_iter_next(it);
        }
    }


    return 1;
}

Buffer *serialize_table(Table *mp) {
    TableIter *it;
    Buffer *tmp;
    Buffer *total_buff;
    List* buff_chain;
    size_t key_len, val_len;

    total_buff = (Buffer *)malloc(sizeof (Buffer));

    it = table_iter_new(mp);
    total_buff->len = 0;
    buff_chain = new_list();
    while (table_iter_next(it)) {
        key_len = it->curr->key.len;
        val_len = it->curr->value.len;

        tmp = (Buffer *)malloc(sizeof (Buffer));
        tmp->len = key_len + sizeof (size_t) + val_len + sizeof (size_t);
        tmp->data = (char *)malloc(sizeof(char) * tmp->len );
        memset(tmp->data, 0, tmp->len);

        memcpy(tmp->data, &key_len, sizeof (size_t));
        memcpy(tmp->data + sizeof (size_t), it->curr->key.data, key_len);
        memcpy(tmp->data + sizeof (size_t) + key_len, &val_len, sizeof (size_t));
        memcpy(tmp->data + sizeof (size_t) + key_len + sizeof (size_t), it->curr->value.data, val_len);

        list_push(buff_chain, tmp);
        total_buff->len += tmp->len;
    }
    total_buff->data = consume_buff_chain(buff_chain, total_buff->len);

    free(it);
    it = NULL;

    return total_buff;
}

DeserializeTableIter *deserialize_table_iter(Buffer *table_buff) {
    DeserializeTableIter *it;
    it = malloc(sizeof(TableIter));
    it->curr_pos = table_buff->data;
    it->end_pos = table_buff->data +table_buff->len;
    it->curr = NULL;
    return it;
}

int deserialize_table_iter_next(DeserializeTableIter *it) {
    if (it->curr_pos >= it->end_pos) {
        return 0;
    }
    if (it->curr == NULL) {
        it->curr = malloc(sizeof(TableNode));
        it->curr->key.len = 0;
        it->curr->key.data = NULL;
        it->curr->value.len = 0;
        it->curr->value.data = NULL;
        it->curr->next = NULL;
    }
    if (it->curr->key.data != NULL) {
        free(it->curr->key.data);
    }
    if (it->curr->value.data != NULL) {
        free(it->curr->value.data);
    }

    memcpy(&it->curr->key.len, it->curr_pos, sizeof (size_t));
    it->curr->key.data = malloc(sizeof (char) * it->curr->key.len);
    memcpy(it->curr->key.data, it->curr_pos + sizeof (size_t), it->curr->key.len);

    memcpy(&it->curr->value.len, it->curr_pos + sizeof (size_t) + it->curr->key.len, sizeof (size_t));
    it->curr->value.data = malloc(sizeof (char) * it->curr->value.len);
    memcpy(it->curr->value.data, it->curr_pos  + sizeof (size_t) + it->curr->key.len + sizeof (size_t), it->curr->value.len);

    it->curr_pos += sizeof (size_t) + it->curr->key.len + sizeof (size_t) + it->curr->value.len;

    return 1;
}

