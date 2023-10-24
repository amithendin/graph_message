//
// Created by amit on 10/15/23.
//
#include <time.h>
#include <locale.h>
#include "util.h"

Buffer buffer_from_str(char* str, short copy) {
    Buffer buff;

    if (copy < 0) {
        buff.len = strlen(str);
        buff.data = malloc(buff.len);
        memcpy(buff.data, str, buff.len);

    }else if (copy == 0) {
        buff.len = strlen(str);
        buff.data = str;

    }else {
        buff.len = copy;
        buff.data = malloc(buff.len);
        memcpy(buff.data, str, buff.len);
    }

    return buff;
}

Buffer *copy_buffer(Buffer *buff) {
    Buffer *copy_buff = (Buffer*) malloc(sizeof (Buffer));
    copy_buff->len = buff->len;
    copy_buff->data = malloc(buff->len);
    memcpy(copy_buff->data, copy_buff->data, buff->len);
    return copy_buff;
}

void free_buffer(Buffer *buff) {
    if (buff->data != NULL) {
        free(buff->data);
    }
    free(buff);
}

short buffer_cmp(Buffer a, Buffer b) {/*MSB at index 0*/
    Uint i;
    char *aptr, *bptr;

    if (a.len < b.len) {
        return -1;
    }else if (a.len > b.len) {
        return 1;
    }

    aptr = (char*)a.data;
    bptr = (char*)b.data;

    for (i=0;i<a.len;i++) {
        if (aptr[i] < bptr[i]) {
            return -1;
        }else if (aptr[i] > bptr[i]) {
            return 1;
        }
    }

    return 0;
}

int random_int(int min, int max) {
    return min + rand() % (max+1 - min);
}

char* gen_peer_id() {
    int i, is_num;
    char *peer_id = (char*) malloc(sizeof (char)*PEER_ID_SIZE+1);
    for(i=0;i<PEER_ID_SIZE;i++) {
        is_num = random_int(0,1);
        if (is_num) {
            peer_id[i] = '0' + random_int(0, 9);
        }else {
            peer_id[i] = 'A' + random_int(0, 25);
        }
    }
    return peer_id;
}

Time now_milliseconds(void) {
    struct timeval tv;

    gettimeofday(&tv,NULL);
    return (((long long)tv.tv_sec)*1000)+(tv.tv_usec/1000);
}

char* miliseconds_to_datestr(Time t) {
    char *isoDateString;
    time_t now;
    struct tm *timeinfo;
    int milliseconds;

    isoDateString = malloc(30);

    now = t/1000;
    milliseconds = t / 1000000000;
    timeinfo = localtime(&now);

    strftime(isoDateString, 30, "%Y-%m-%d_%H:%M:%S", timeinfo);
    snprintf(isoDateString + strlen(isoDateString), 5, ".%d", milliseconds);

    return isoDateString;
}