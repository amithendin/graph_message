//
// Created by amit on 8/13/23.
//

#ifndef DISTMSG_TABLE_H
#define DISTMSG_TABLE_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "util.h"

#define TABLE_SIZE 100
#define START_HASH 5321
#define HASH_SHIFT 5

typedef struct node {
    Buffer key;
    Buffer value;
    struct node *next;
} TableNode;

typedef struct table {
    unsigned int size;
    TableNode *arr[TABLE_SIZE];
} Table;

typedef struct {
    int i;
    TableNode *curr;
    Table *mp;
} TableIter;

typedef struct {
    TableNode *curr;
    char *curr_pos;
    char *end_pos;
} DeserializeTableIter;

Table *new_table();

void free_table(Table *mp);

int table_hash(Table *mp, Buffer key);

void table_insert(Table *mp, Buffer key, Buffer value);

void table_delete(Table *mp, Buffer key);

Buffer *table_search(Table *mp, Buffer key);

TableIter* table_iter_new(Table *mp);

int table_iter_next(TableIter *it);

Buffer *serialize_table(Table *mp);

DeserializeTableIter *deserialize_table_iter(Buffer *table_str);

int deserialize_table_iter_next(DeserializeTableIter *it);

#endif //DISTMSG_TABLE_H
