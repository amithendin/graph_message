//
// Created by amit on 8/28/23.
//

#ifndef DISTMSG_MESSAGE_H
#define DISTMSG_MESSAGE_H

#include <stdlib.h>
#include <netinet/in.h>
#include <string.h>
#include "util.h"

typedef struct {
    Time time;
    char from_peer[PEER_ID_SIZE];
    char to_peer[PEER_ID_SIZE];
    char through_peer[PEER_ID_SIZE];
    Buffer content;

} Message;

Message *new_message(Buffer *content, char *from, char *to);

Message *copy_message(Message* msg);

void free_message(Message *msg);

Buffer* serialize_msg(Message *m);

Message* deserialize_msg(char* buff);

Buffer gen_message_signature(Message *m);

#endif //DISTMSG_MESSAGE_H
