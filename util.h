//
// Created by amit on 8/28/23.
//

#ifndef DISTMSG_UTIL_H
#define DISTMSG_UTIL_H

#define BUFFER_SIZE 1024
#define PEER_ID_SIZE 8

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include<sys/time.h>


typedef unsigned int Uint;

typedef long long Time;

typedef struct {
    Uint len;
    void *data
} Buffer;

Buffer buffer_from_str(char* str, short copy);

Buffer *copy_buffer(Buffer *buff);

void free_buffer(Buffer *buff);

short buffer_cmp(Buffer a, Buffer b);

int random_int(int min, int max);

char* gen_peer_id();

Time now_milliseconds(void);

char* miliseconds_to_datestr(Time t);

#endif //DISTMSG_UTIL_H
