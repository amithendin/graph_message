/**
 * Author: Amit Hendin
 * Date: 15/10/2023
 *
 * implements utils.h
 */
#include <time.h>
#include <locale.h>
#include "util.h"

Buffer buffer_from_str(char* str, short copy) {
    Buffer buff;

    if (copy < 0) {
        /* Allocate space for the content of str and then copy all bytes from str to buff.data */
        buff.len = strlen(str);
        buff.data = malloc(buff.len);
        memcpy(buff.data, str, buff.len);

    }else if (copy == 0) {
        /* Just copy the pointer and the length */
        buff.len = strlen(str);
        buff.data = str;

    }else {
        /* Allocate <copy> bytes and copy <copy> bytes from str */
        buff.len = copy;
        buff.data = malloc(buff.len);
        memcpy(buff.data, str, buff.len);
    }

    return buff; /* return the newly created buffer */
}

void free_buffer(Buffer *buff) {
    if (buff->data != NULL) { /* If buffer points to data, free data as well */
        free(buff->data);
    }
    free(buff);
}

short buffer_cmp(Buffer a, Buffer b) {/*MSB at index 0*/
    Uint i;
    char *aptr, *bptr;

    /* Here we treate buffers like large numbers with each byte as a digit, in this logic if a buffer has more bytes that another
     * it is greater by "an order of magnitude" since as a number, it has more digits */
    if (a.len < b.len) {
        return -1;
    }else if (a.len > b.len) {
        return 1;
    }

    aptr = (char*)a.data;
    bptr = (char*)b.data;

    for (i=0;i<a.len;i++) { /* Treat the bytes at index 0 as most significant digit of the number, compare lie in radix sort */
        if (aptr[i] < bptr[i]) {
            return -1;
        }else if (aptr[i] > bptr[i]) {
            return 1;
        }
    }

    return 0; /* if we didn;t return 1 or -1 than buffer contents are equal */
}

int random_int(int min, int max) {
    return min + rand() % (max+1 - min);
}

char* gen_peer_id() {
    int i, is_num;
    char *peer_id = (char*) malloc(sizeof (char)*PEER_ID_SIZE+1); /* Allocate space for peer id and null terminator */
    for(i=0;i<PEER_ID_SIZE;i++) {
        /* For each cell in the peer id there is a 50% chance to be a digit and 50% chance to be a capital case english letter
         * therefore we first generate random number between 0 and 1, then a random number in the respective range (0-9 or 0-25)*/
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