//
// Created by amit on 8/28/23.
//
#include "message.h"

Message *new_message(Buffer* content, char *from, char *to) {
    Message *msg = malloc(sizeof (Message));
    msg->content.len = content->len;
    msg->content.data = malloc(msg->content.len);
    memcpy(msg->content.data, content->data, msg->content.len);
    strncpy(msg->to_peer, to, PEER_ID_SIZE);
    strncpy(msg->from_peer, from, PEER_ID_SIZE);
    memset(msg->through_peer, 0, PEER_ID_SIZE);
    msg->time = now_milliseconds();
    return msg;
}

Message *copy_message(Message* msg) {
    Message *copy_msg = malloc(sizeof (Message));
    copy_msg->content.len = msg->content.len;
    copy_msg->content.data = (char*) malloc(copy_msg->content.len);
    strncpy(copy_msg->content.data, msg->content.data, msg->content.len);
    strncpy(copy_msg->to_peer, msg->to_peer, PEER_ID_SIZE);
    strncpy(copy_msg->from_peer, msg->from_peer, PEER_ID_SIZE);
    memset(copy_msg->through_peer, 0, PEER_ID_SIZE);
    return copy_msg;

}

void free_message(Message *msg) {
    free(msg->content.data);
    free(msg);
}

Buffer gen_message_signature(Message *m) {
    Buffer sgn;
    sgn.len = PEER_ID_SIZE*2+sizeof (Time);
    sgn.data = malloc(sgn.len);
    memcpy(sgn.data, m->from_peer, PEER_ID_SIZE);
    memcpy(sgn.data + PEER_ID_SIZE, m->to_peer, PEER_ID_SIZE);
    memcpy(sgn.data + (PEER_ID_SIZE*2), &m->time, sizeof(Time) );
    return sgn;
}

Buffer* serialize_msg(Message *m) {
    Buffer *buff;
    buff = (Buffer *)malloc(sizeof (Buffer));
    buff->len = sizeof(Time)+ 2*PEER_ID_SIZE + sizeof (Uint) + m->content.len;/*peer_ids + content_len + content_data*/
    buff->data = (char*)malloc(buff->len);

    memcpy(buff->data, &m->time, sizeof(Time));
    memcpy(buff->data+sizeof(Time), m->from_peer, PEER_ID_SIZE);
    memcpy(buff->data+sizeof(Time)+PEER_ID_SIZE, m->to_peer, PEER_ID_SIZE);

    memcpy(buff->data+sizeof(Time)+2*PEER_ID_SIZE, &m->content.len, sizeof(Uint));
    memcpy(buff->data+sizeof(Time)+2*PEER_ID_SIZE+sizeof(Uint), m->content.data, m->content.len);

    return buff;
}

Message* deserialize_msg(char* buff) {
    Message *msg;

    msg = malloc(sizeof (Message));

    memcpy(&msg->time, buff, sizeof (Time));
    memcpy(msg->from_peer, buff+sizeof (Time), PEER_ID_SIZE);
    memcpy(msg->to_peer, buff+sizeof (Time) + PEER_ID_SIZE, PEER_ID_SIZE);

    memcpy(&msg->content.len, buff +sizeof (Time)+ 2*PEER_ID_SIZE, sizeof (Uint));
    msg->content.data = malloc(msg->content.len);
    memcpy(msg->content.data, buff +sizeof (Time) + 2*PEER_ID_SIZE + sizeof (Uint), msg->content.len);
    memset(msg->through_peer, 0, PEER_ID_SIZE);

    return msg;
}