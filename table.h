/**
 * Hashtable implementation with chaining
 * Author: Amit Hendin
 * Date: 8/13/2023
 *
 * A standard and simple implementation of a hashtable data structure using chaining to resolve conflicts and with some extra features
 */

#ifndef DISTMSG_TABLE_H
#define DISTMSG_TABLE_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "util.h"

#define TABLE_SIZE 100 /* number of cells in the hash table array */
#define START_HASH 5321 /* parameter for the hash function */
#define HASH_SHIFT 5 /* another parameter */

/**
 * Holds a node in the hash table which contains a single key value pair and pointer to the next key in the chain
 */
typedef struct node {
    Buffer key;
    Buffer value;
    struct node *next;
} TableNode;
/**
 * Holds the entire table basically
 */
typedef struct table {
    unsigned int size;
    TableNode *arr[TABLE_SIZE];
} Table;
/**
 * Holds the data necessary to iterate over the table completely
 */
typedef struct {
    int i;
    TableNode *curr;
    Table *mp;
} TableIter;
/**
 * Holds the data necessary to iterate over the table completely where the table is encoded in a byte buffer
 */
typedef struct {
    TableNode *curr;
    char *curr_pos;
    char *end_pos;
} DeserializeTableIter;
/**
 * Creates a new empty table
 *
 * @return Pointer to the new table
 */
Table *new_table();
/**
 * Frees the table and all memory it references except for the values
 *
 * @param mp
 */
void free_table(Table *mp);
/**
 * The hash function used to calculate the array index from a provided key
 *
 * @param mp Pointer to the table
 * @param key Buffer which contains the key
 * @return The index in the array which calculated from the key
 */
int table_hash(Table *mp, Buffer key);
/**
 * Insert a key and value to a table
 *
 * @param mp Pointer to the table
 * @param key Buffer which contains the key
 * @param value Buffer which contains the value
 */
void table_insert(Table *mp, Buffer key, Buffer value);
/**
 * Delete value from the table by it's key
 *
 * @param mp Pointer to the table
 * @param key Buffer containing the key
 */
void table_delete(Table *mp, Buffer key);
/**
 * Search for a value in the table by it's key
 *
 * @param mp Pointer to the table
 * @param key Buffer containing the key
 * @return A pointer to the buffer containing the value paired to the key
 */
Buffer *table_search(Table *mp, Buffer key);
/**
 * Create an iterator that starts from the begging of the table
 *
 * @param mp Pointer to the table
 * @return An pointer to a new iterator struct which points to the begging of the table
 */
TableIter* table_iter_new(Table *mp);
/**
 * Move the give table iterator to the next key/value pair in the table
 *
 * @param it Pointer to the iterator
 * @return 1 if there is another key/value pair in the table, otherwise 0
 */
int table_iter_next(TableIter *it);
/**
 * Creates a new buffer of bytes which encodes all the data of the table in it
 *
 * @param mp Pointer to a table to encode
 * @return Pointer to the newly created buffer in which the given table is encoded
 */
Buffer *serialize_table(Table *mp);
/**
 * Creates a deserializing iterator struct for a buffer containing a serialized table
 *
 * @param table_str The buffer in which a table is encoded
 * @return Pointer to a deserializing iterator struct which points to the begining of the buffer
 */
DeserializeTableIter *deserialize_table_iter(Buffer *table_str);
/**
 * Move the give deserializing iterator to the next key/value pair in the buffer
 *
 * @param it Pointer to a deserializing iterator struct
 * @return 1 if there is another key/value pair in the buffer, otherwise 0
 */
int deserialize_table_iter_next(DeserializeTableIter *it);

#endif //DISTMSG_TABLE_H
