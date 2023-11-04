/**
 * Author: Amit Hendin
 * Date: 28/8/2023
 *
 * Various utility functions and general use structs and typedefs
 */

#ifndef DISTMSG_UTIL_H
#define DISTMSG_UTIL_H

#define BUFFER_SIZE 1024
#define PEER_ID_SIZE 8

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include<sys/time.h>

typedef unsigned int Uint;

typedef long long Time; /* Must be big enough to hold time in ms*/
/* Holds a pointer to a memory buffer of any type the the length of the buffer in bytes */
typedef struct {
    Uint len;
    void *data
} Buffer;
/**
 * Creates a buffer from a given string
 *
 * @param str String to create a buffer from
 * @param copy 0 - assigns the given pointer directly to the data pointer,
 * 1 or greater - the length of bytes to copy from the given pointer's content to the new pointer's content,
 * less then 0 - copy the entire content of the given pointer to a new pointer
 * @return A new buffer struct containing the length and pointer to the data
 */
Buffer buffer_from_str(char* str, short copy);
/**
 * Frees the buffer pointer and memory it points to
 *
 * @param buff Pointer to the buffer struct to free
 */
void free_buffer(Buffer *buff);
/**
 * Compares two buffers by length and bytes wise comparison
 *
 * @param a First buffer to compare
 * @param b Second buffer to compare
 * @return 0 - if a and b hold equal content, 1 if content in a is greater than b, -1 otherwise.
 */
short buffer_cmp(Buffer a, Buffer b);
/**
 * Returns a random integer between a given range
 *
 * @param min Minimum value to return
 * @param max Maximum value to return
 * @return Random integer between min and max
 */
int random_int(int min, int max);
/**
 * Generate a random peer id
 *
 * @return New randomly generated peer id
 */
char* gen_peer_id();
/**
 * Returns the current time in milliseconds
 *
 * @return The current time in milliseconds
 */
Time now_milliseconds(void);
/**
 * Create ISO format date-time string from milliseconds timestamp
 *
 * @param t Time in milliseconds
 * @return Date-time string
 */
char* miliseconds_to_datestr(Time t);

#endif //DISTMSG_UTIL_H
