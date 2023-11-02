/***
 * Distributed Peer to Peer Messaging
 * Author: Amit Hendin
 * Date: 31/10/2023
 *
 * The purpose of this program is to enable users to create a distrubuted messaging network and pass messages between each other through each other.
 * This program works in the following manner, running this program creates a vertex in a graph. Running the "connect" command create an edge between
 * another peer supplied to the connect command. A vertex/peer can send messages to any other peer so long there is a path between them.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <locale.h>
#include <errno.h>

#include "util.h"
#include "table.h"
#include "list.h"
#include "message.h"
#include "config.h"

/**
 * Command codes
 * */
#define CMD_DISCOVER 1
#define CMD_SEND 2
#define CMD_CONNECT 3
#define CMD_FETCH_INBOX 4

/**
 * A command that is recieved from the interface client through the interface server.
 */
typedef struct {
    char cmd; /* Command code */
    char peer_id[PEER_ID_SIZE];/* Peer ID */
    unsigned long long content_len;/* length of the content of the "content" pointer */
    char *content;/* pointer to the content bytes of the command which given the current command set is only content of message */
} Command;

/**
 * A response from this program to a client interface
 */
typedef struct {
    Time time; /* The time in miliseconds when the response was created */
    char from_peer[PEER_ID_SIZE]; /* The peer which the response came from (could be this program could be from a message recieved)*/
    unsigned long long content_len; /* length of the content of the "content" pointer */
    char *content;/* pointer to the content bytes of the response which could be a message from another peer or messages from this program to the client */
} ClientResponse;

/**
 * outbox - queue of messages to send
 * inbox - queue of messages to read, some be not be for "me" so I'll broadcast them to all my neighbors
 * personal_inbox - queue those messages from the inbox that have "me" and the to_peer property of the message
 * message_table - table of messages I've recieved weather for me or not so that I can ignore when i get the same message from multiple sources
 * outbox_mutex, inbox_mutex, message_table_mutex, personal_inbox_mutex - mutexes to handle their respective queues/tables to share among threads
 * conf - configuration struct with all the config variables interpreted from the config file
 */
List *outbox, *inbox, *personal_inbox;
Table *message_table;
pthread_mutex_t outbox_mutex, inbox_mutex, message_table_mutex, personal_inbox_mutex;
Config conf;

void server();
void client();

/**
 * Deserializes a commmand from a buffer of bytes
 *
 * @param buff a buffer of bytes in which a Command struct is encoded
 * @param buff_len the length of buff in bytes
 * @return The command encoded in the byte in buff
 */
Command deserialize_command(char *buff, size_t buff_len) {
    Command cmd;/* Command sturct to hold the deserialized result  */

    cmd.cmd = buff[0]; /* first byte is always the command code */
    buff_len -= 1; /* we read 1 bytes, so the remaining bytes to read are reduced by 1 */
    memcpy(cmd.peer_id, buff+1, PEER_ID_SIZE); /* copy the peer id from the buffer, comes directly after the command code */
    buff_len -= PEER_ID_SIZE;/* again, we read PEER_ID_SIZE bytes therefore we have 8 less total bytes left */
    cmd.content_len = buff_len;/* the rest of the bytes in the buffer are all content bytes, there for the remaining bytes will be the size of the content */
    if (buff_len > 0) { /* make sure we even have content/bytes left to read */
        cmd.content = malloc(buff_len);/* allocate space for the content and read the rest of the bytes */
        memcpy(cmd.content, buff + 1 + PEER_ID_SIZE, buff_len);
    }

    return cmd;/*return result*/
}

/**
 * Serialize ClientResponse struct into a buffer of bytes
 *
 * @param resp A ClientResponse struct to serialize
 * @return A buffer which contains the bytes encoding of in given ClientResponse
 */
Buffer serialize_response(ClientResponse *resp) {
    Buffer buf;/* a buffer to hold the serialized bytes */

    buf.len = sizeof(Time) + PEER_ID_SIZE + sizeof(unsigned long long) + resp->content_len; /* calculate how many bytes the buffer needs to hold the ClientResponse struct */
    buf.data = malloc(buf.len); /* allocate space for the buffer */

    memcpy(buf.data, &resp->time, sizeof(Time)); /* copy the time data to the start of the buffer */
    memcpy(buf.data+sizeof(Time), resp->from_peer, PEER_ID_SIZE); /* next comes the from_peer peer id buffer */
    memcpy(buf.data+sizeof(Time)+PEER_ID_SIZE, &resp->content_len, sizeof(unsigned long long ));/* next comes the size of the content */
    memcpy(buf.data+sizeof(Time)+PEER_ID_SIZE+sizeof(unsigned long long ), resp->content, resp->content_len); /* lastly comes the content */

    return buf; /* return the encoded bytes */
}

/**
 * Push a message into a given queue
 *
 * @param queue_mutex The mutex of the queue to push into
 * @param queue The pointer to the queue
 * @param msg The pointer to the message
 */
void enqueue_message(pthread_mutex_t *queue_mutex, List *queue, Message *msg) {
    /* Pretty self-explanatory, check that we can write to queue using the mutex, then write to queue */
    pthread_mutex_lock(queue_mutex);
    list_push(queue, msg);
    pthread_mutex_unlock(queue_mutex);
}

/**
 * Pop a message from a given queue
 *
 * @param queue_mutex The mutex of the queue to pop from
 * @param queue The pointer to the queue
 * @return A pointer to the message poped from queue
 */
Message* dequeue_message(pthread_mutex_t *queue_mutex, List *queue) {
    Message *msg;
    /* Pretty self-explanatory, check that we can write to queue using the mutex, then write to queue */
    pthread_mutex_lock(queue_mutex);
    msg = (Message *) list_poplast(queue); /* preforms pop operation but from the back of the queue to achieve the First In First Out order */
    pthread_mutex_unlock(queue_mutex);

    return msg;
}

/**
 * Sends a message to another instance of this program over the internet
 *
 * @param addr IP address of the other instance
 * @param port Port number the other instance of this program is listening on
 * @param msg Pointer to the message to send
 * @return 1 is there was an error, 0 if sent successfully
 */
int net_client(char *addr, int port, Message *msg) {
    int sock;
    struct sockaddr_in server;
    Buffer *buff; /* to hold serialized message*/

    buff = serialize_msg(msg); /* serialize the message to bytes */

    /* Create socket */
    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock == -1) {
        printf("failed to create socket\n");
    }
    /* Set address and port of target */
    server.sin_addr.s_addr = inet_addr(addr);
    server.sin_family = AF_INET;
    server.sin_port = htons(port);

    /* Connect to target */
    if (connect(sock, (struct sockaddr *) &server, sizeof(server)) < 0) {
        perror("connect failed\n");
        free(buff);
        return 1;
    }

    /* Send the serialized message to the target */
    if (send(sock, buff->data, buff->len, 0) < 0) {
        printf("send failed\n");
        free(buff); /* If the message has failed we return 1, therefore we must free the serialized message buffer first to avoid memory leak */
        return 1;
    }

    close(sock); /* Close the connection properly */

    free(buff); /* free the serialized message buffer */
    return 0;
}

/**
 * Thread function that listens on the configured port for messages from other instances of this program
 *
 * @param vargp Standard thread program argument pointer
 * @return Never
 */
void *net_server(void *vargp) {
    int listenfd, client_sock;
    long read_size;
    struct sockaddr_in serv_addr;
    char buff[BUFFER_SIZE]; /* buffer to recieve bytes into */
    Buffer *tmp, *total_buff; /* total_buff - to hold the accumulated bytes recieved from the connection */
    List *buff_chain; /* chain of buffers accumulated in the recieve loop */
    Message *msg; /* message recieved from deserializing the accumulated buffers */

    listenfd = socket(AF_INET, SOCK_STREAM, 0); /* Create socket */
    /* Set port and address*/
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    serv_addr.sin_port = htons(conf.port);

    bind(listenfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr));

    /* listen on socket */
    if (listen(listenfd, 10) == -1) {
        printf("failed to listen\n");
        return NULL;
    }

    while (1) {
        client_sock = accept(listenfd, (struct sockaddr *) NULL, NULL); /* Accept an incoming connection */
        if (client_sock < 0) {
            perror("accept failed\n");
            return NULL;
        }

        /* Init buffers form accumulating recieved bytes */
        total_buff = (Buffer *) malloc(sizeof(Buffer));
        total_buff->len = 0;
        buff_chain = new_list();
        while ((read_size = recv(client_sock, buff, BUFFER_SIZE, 0)) > 0) {/* Recieve bytes from connection */
            tmp = malloc(sizeof (Buffer));
            tmp->len = read_size;
            tmp->data = malloc(read_size);
            memcpy(tmp->data, buff, read_size);
            list_push(buff_chain, tmp); /* add recieved buffer to the buffer chain */
            total_buff->len += read_size;
        }
        total_buff->data = consume_buff_chain(buff_chain, total_buff->len); /* turn buffer chain into one large buffer */
        msg = deserialize_msg(total_buff->data); /* deserialize that large buffer into a message struct */

        enqueue_message(&inbox_mutex, inbox, msg); /* push the message into the inbox */

        server(); /* trigger the handeling of messages in the inbox */

        free_buffer(total_buff); /* free the total buffer */
        memset(buff, 0, BUFFER_SIZE); /* zero the recieve buffer */
        close(client_sock); /* close connection */
    }

    return NULL;
}

/**
 * Handles the messages in the outbox queue
 */
void client() {
    /* Variables to hold various temporary data */
    int i, port;
    char addr[BUFFER_SIZE], *tmp;
    Message *msg;
    Buffer peer_req_data, *tmp_buf, tmp_peer_id;

    do {
        tmp = NULL;
        msg = NULL;
        peer_req_data.len = 0;
        peer_req_data.data = NULL;

        msg = dequeue_message(&outbox_mutex, outbox); /* pop a message from the outbox queue */

        if (msg != NULL) { /* If indeed a message was poped from the queue */
            if (msg->through_peer[0]) { /* Check if the message has a through peer defined, since through peer buffer is zeroed by default, it is enough to check the first byte */
                tmp_peer_id = buffer_from_str(msg->through_peer,
                                              PEER_ID_SIZE); /* set peer id to look for in peer table to the through peer of the message*/
            } else {
                tmp_peer_id = buffer_from_str(msg->to_peer,
                                              PEER_ID_SIZE); /* otherwise, set peer id to look for in peer table to the to peer of the message*/
            }

            tmp_buf = table_search(conf.peer_table, tmp_peer_id); /* search for target peer in the peer table */

            if (tmp_buf != NULL) {
                /* if the target peer was found in the peer table, parse his IP and port from the peer table search result and send the message */
                tmp = (char *) tmp_buf->data; /* address is stored as xxxx.xxxx.xxxx.xxxx:ppppppp therefore we collect all characters before : to the IP address string and all characters after to the port number*/
                for (i = 0; tmp[i] != 0; i++) {
                    if (tmp[i] == ':') {
                        tmp = tmp + i + 1;
                        addr[i] = 0;
                        break;
                    }
                    addr[i] = tmp[i];
                }
                port = atoi(tmp);
                net_client(addr, port, msg); /* send the message to the target */
                free_message(msg); /* free the message since it's not going back into any queue */

            } else { /*Otherwise, we want to broadcast the message to all our neighbors (meaning all peers in our peer table). We do this by artificially inserting the message
 * into our inbox which will cause the server function to broadcast it since we know that the peer is not found in the peer table*/
                enqueue_message(&inbox_mutex, inbox, msg);
                server();/* trigger the handling of the inbox queue */
            }

        }
    } while (msg != NULL);
}

/**
 * Handles messages in the inbox queue
 */
void server() {
    /* Variables to hold various temporary data */
    Message *msg, *discover_msg, *broadcast_msg;
    TableIter *it;
    DeserializeTableIter *de_it;
    Buffer *table_buf;
    Buffer sgn, *lookup, new_buf;
    char *time_str;
    ClientResponse *resp;

    do {
        msg = dequeue_message(&inbox_mutex, inbox);/* pop a message from the outbox queue */

        if (msg != NULL) { /* If a message exists check if it's a discover message */
            if (strncmp(msg->from_peer, "discover", PEER_ID_SIZE) == 0) {
                /* If the message is delivering a neightbors peer table, deserialize the table iteratively, use the deserialize_table_iter
                 * to iterate over the message content buffer where in each iterating a key value pair is returned for the buffers bytes */
                de_it = deserialize_table_iter(&msg->content);

                while (deserialize_table_iter_next(de_it)) { /* while there are still key value pairs in the buffer */
                    table_insert(conf.peer_table, de_it->curr->key,
                                 de_it->curr->value); /* insert them into the peer table */
                }
                /* free everything since we are done handling the message */
                free(de_it);
                de_it = NULL;
                free_message(msg);

            } else if (strncmp(msg->to_peer, "discover", PEER_ID_SIZE) == 0) {
                /* If the message is requesting the peer table, serizlize the peer table, put in the content buffer of a new mesage and push it into the outbox queue*/
                table_buf = serialize_table(conf.peer_table);
                discover_msg = new_message(table_buf, "discover", msg->from_peer);

                enqueue_message(&outbox_mutex, outbox, discover_msg);

                client();/* invoke the handling of the outbox queue */
                /* free the used memory */
                free_buffer(table_buf);
                free_message(msg);

            } else {

                sgn = gen_message_signature(
                        msg); /* generate message signature from message which uniquely identifies the message with a fixed amount of bytes */
                lookup = table_search(message_table,
                                      sgn); /* check is the message is already present in the message table meaning it already passed through here */

                if (lookup == NULL) {
                    /* if it has not passed through here, handle the message */
                    new_buf.data = NULL;
                    new_buf.len = 0;
                    table_insert(message_table, sgn,
                                 new_buf);/* first mark the message as having passed through here by inserting it into the messages table */

                    if (strncmp(msg->to_peer, conf.peer_id, PEER_ID_SIZE) ==
                        0) { /* check if the message is meant for "me", if it is, */

                        if (conf.interface_port) {
                            /* if an interface port is defined, create from a ClientResponse struct from the message to send to the interface client and push it to the personal inbox queue */
                            resp = malloc(sizeof(ClientResponse));
                            resp->time = msg->time;
                            memcpy(resp->from_peer, msg->from_peer, PEER_ID_SIZE);
                            resp->content_len = msg->content.len;
                            resp->content = malloc(resp->content_len);
                            memcpy(resp->content, msg->content.data, resp->content_len);

                            pthread_mutex_lock(&personal_inbox_mutex);
                            list_push(personal_inbox, resp);
                            pthread_mutex_unlock(&personal_inbox_mutex);

                        } else {
                            /* otherwise, simple print the message to the standart out stream */
                            time_str = miliseconds_to_datestr(msg->time);
                            printf("%.*s> [%s] \"%s\"\n", PEER_ID_SIZE, msg->from_peer, time_str,
                                   (char *) msg->content.data);
                            free(time_str);
                        }

                    } else {
                        /*Otherwise, if the message is not meant for "me", broadcast the message to all "my" neighbors */
                        it = table_iter_new(conf.peer_table);

                        while (table_iter_next(
                                it)) { /* iterate over every peer in the peer table and send the message to through that peer by adding a through peer buffer with that peer's id to copy of the orignal message*/
                            if (strncmp((char *) it->curr->key.data, conf.peer_id, PEER_ID_SIZE) != 0 &&
                                strncmp((char *) it->curr->key.data, msg->from_peer, PEER_ID_SIZE) != 0) {
                                broadcast_msg = new_message(&msg->content, msg->from_peer, msg->to_peer);
                                memcpy(broadcast_msg->through_peer, it->curr->key.data, PEER_ID_SIZE);
                                broadcast_msg->time = msg->time;
                                enqueue_message(&outbox_mutex, outbox, broadcast_msg);
                            }
                        }
                        /* invoke handling the outbox */
                        client();

                        free(it);
                        it = NULL;
                    }
                }
                /* once the message is handled, free it */
                free_message(msg);
                free(sgn.data);
            }
        }
    } while(msg != NULL);
}

/**
 * Execute a command received from the interface client
 *
 * @param cmd The command struct to execute
 */
void execute_command(Command cmd) {
    /* Variables to hold various temporary data */
    char peer_id[PEER_ID_SIZE+1], *tmp_str;
    Message *msg, *discover_msg;
    Buffer tmp, *table_buf;
    TableIter *it;
    ClientResponse *resp;

    if (cmd.cmd == CMD_CONNECT) { /* If recieved a connect command, take peer id from command peer id and peer address from command content and add it to the peer table, then send a discover message to that peer
 * so that peer will also add "me" to it's peer table */
        table_buf = serialize_table(conf.peer_table);
        table_insert(conf.peer_table, buffer_from_str(cmd.peer_id, 0), buffer_from_str(cmd.content, 1));

        discover_msg = new_message(table_buf, "discover", peer_id);
        free_buffer(table_buf);

        enqueue_message(&outbox_mutex, outbox, discover_msg);
        client();
        tmp_str = "connect executed";

    }else if (cmd.cmd == CMD_DISCOVER) {/* If recieved a discover command, broadcast a discover message to all peers in "my" peer table */
        tmp = buffer_from_str("0", 1);

        it = table_iter_new(conf.peer_table);

        while (table_iter_next(it)) {
            if (strncmp((char*)it->curr->key.data, conf.peer_id, PEER_ID_SIZE) != 0) {
                discover_msg = new_message(&tmp, conf.peer_id, "discover");
                memcpy(discover_msg->through_peer, it->curr->key.data, PEER_ID_SIZE);
                enqueue_message(&outbox_mutex, outbox, discover_msg);
            }
        }
        /* invoke handling outbox */
        client();

        free(tmp.data);
        free(it);
        it = NULL;
        tmp_str = "discover executed";

    }else if (cmd.cmd == CMD_SEND) {/* If recieved a send command, take peer id from command peer id and take message content from command content and create with them a message and push it to the outbox */
        tmp.len = cmd.content_len;
        tmp.data = cmd.content;
        msg = new_message(&tmp, conf.peer_id, cmd.peer_id);
        enqueue_message(&outbox_mutex, outbox, msg);
        /* invoke handling outbox */
        client();
        tmp_str = "send executed";

    }else {
        tmp_str = "unrecognized command";
    }

    /* Form a ClientResponse struct and send it to the interface client to notify it we recieved and executed the command it sent */
    resp = malloc(sizeof(ClientResponse));
    resp->time = now_milliseconds();
    memcpy(resp->from_peer, "PEERCLNT", PEER_ID_SIZE); /* PEERCLNT is short for peer client, refering to this program instance and a client to other peers in the network */
    resp->content_len = strlen(tmp_str);
    resp->content = malloc(resp->content_len);
    memcpy(resp->content, tmp_str, resp->content_len);

    /* Add ClientResponse to personal inbox in order to send it to the interface client */
    pthread_mutex_lock(&personal_inbox_mutex);
    list_push(personal_inbox, resp);
    pthread_mutex_unlock(&personal_inbox_mutex);
}

size_t find_buf_len(char buff[BUFFER_SIZE]) {
    size_t i;
    i=BUFFER_SIZE;
    while(buff[i]==0 && i > 0)
        i--;
    return i;
}

/**
 * Thread function that listens on the configured interface port for commands from a client program which knows the expected data communication format
 *
 * @param vargp Standard thread program argument pointer
 * @return Never
 */
void *remote_interface(void *varpg) {
    /* Variables to hold various temporary data */
    int server_socket, interface_client_socket, break_loop;
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_addr_len = sizeof(client_addr);
    char buff[BUFFER_SIZE], *total_buff;
    size_t total_buff_len, total_buff_offset, buff_len;
    Command cmd;
    ClientResponse *resp;
    Buffer serialized_resp;

    /* Create socket */
    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    /* Set port */
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(conf.interface_port);
    server_addr.sin_addr.s_addr = INADDR_ANY;

    bind(server_socket, (struct sockaddr *)&server_addr, sizeof(server_addr));

    /* Listen for incoming connections */
    listen(server_socket, 5);

    while (1) {
        interface_client_socket = accept(server_socket, (struct sockaddr *)&client_addr, &client_addr_len); /* accept an incoming connection */
        break_loop = 0;

        while (!break_loop) { /* Keep looping while the connection is alive */
            total_buff_len = total_buff_offset = 0;
            memset(buff, 0, BUFFER_SIZE);
            buff_len = 0;
            /* Recieve bytes from the interface client */
            while ((buff_len = recv(interface_client_socket, buff, BUFFER_SIZE, MSG_DONTWAIT)) > 0) {
                //buff_len = find_buf_len(buff);
                if (total_buff_len == 0) { /* first sizeof(size_t) bytes are a size_t number that is the total length of the bytes send from the client*/
                    memcpy(&total_buff_len, buff, sizeof(size_t)); /* read the total length of bytes */
                    if (total_buff_len == 0) {
                        break;
                    }
                    total_buff = malloc(total_buff_len); /* allocate space for all the bytes we are about to recieve*/
                    if (buff_len > sizeof(size_t)) { /* if we received more bytes, put them into the new buffer */
                        buff_len -= sizeof(size_t); /* if we recieve n bytes, we send n+sizeof(size_t) bytes so the total length of bytes left to recieve is n+sizeof(size_t)-sizeof(size_t)=n*/
                        memcpy(total_buff, buff + sizeof(size_t), buff_len);
                        total_buff_offset += buff_len;/* keep track of where in the buffer of total bytes are we recieving to */
                    }
                }else {
                    /* Take in the new bytes into the buffer of all the bytes received */
                    memcpy(total_buff+total_buff_offset, buff, buff_len);
                    total_buff_offset += buff_len;
                }
                memset(buff, 0, BUFFER_SIZE);
                buff_len = 0;

                if (total_buff_offset >= total_buff_len - sizeof(size_t)) { /* Once the total amount of bytes recieved equals/exceeds the total we expect to recieve, we deserialize
 * a command struct from those bytes and execute that command freeing space the moment it is no longer needed as we go*/
                    cmd = deserialize_command(total_buff, total_buff_offset);
                    free(total_buff);
                    total_buff_len = total_buff_offset = 0;

                    execute_command(cmd);

                    if (cmd.content_len > 0) {
                        free(cmd.content);
                        cmd.content_len = 0;
                    }

                    break;/* break out of the recieve loop because we finished recieving the message */
                }
            }

            if (!(errno == EAGAIN || errno == EWOULDBLOCK)) {
                break_loop = 1;
            }

            do { /* Read messages from the personal inbox and send them to the interface client */
                pthread_mutex_lock(&personal_inbox_mutex);
                resp = (ClientResponse *)list_poplast(personal_inbox);
                pthread_mutex_unlock(&personal_inbox_mutex);

                if (resp != NULL) {/* If we have messages in the inbox to send to the client, put them into a buffer with a buffer length prefixed and send them off to the client */
                    serialized_resp = serialize_response(resp);
                    free(resp->content);
                    free(resp);
                    /* In order to send the serialized ClientResponse to the client we use the same technique where we send the bytes + size_t number that contains the length of bytes
                     * and the client is expected to have and indentical ClientResponse struct to  deserialize into and to know thta the first sizeof(size_t) bytes of the length of the
                     * total bytes*/
                    total_buff_len = serialized_resp.len + sizeof(size_t);
                    total_buff = malloc(total_buff_len);
                    memcpy(total_buff, &total_buff_len, sizeof(size_t));
                    memcpy(total_buff + sizeof(size_t), serialized_resp.data, total_buff_len - sizeof(size_t));
                    free(serialized_resp.data);

                    if (send(interface_client_socket, total_buff, total_buff_len, 0) < 0) {
                        /* If the message has failed we have lost connection to the client and therefore we break the recieve loop */
                        printf("interface send failed\n");
                        free(total_buff);
                    }
                    free(total_buff);
                    total_buff_len = total_buff_offset = 0;
                }
            }while(resp != NULL);

            sleep(1);
        }

        close(interface_client_socket);
    }
    close(server_socket);

    return 0;
}

int main(int argc, char *argv[]) {
    /* Necessary threads */
    pthread_t server_tid, interface_tid;
    /*Seed random based on time*/
    srand ( time(NULL) );

    if ( !load_config(&conf,argv[1]) ) { /* make sure we have a valid configuration file */
        printf("CANNOT START WITHOUT VALID CONFIG FILE\n");
        return 1;
    }

    setlocale(LC_TIME, conf.locale); /* set the locale */
    /* init the global variables */
    pthread_mutex_init(&outbox_mutex, NULL);
    pthread_mutex_init(&inbox_mutex, NULL);
    pthread_mutex_init(&message_table_mutex, NULL);
    pthread_mutex_init(&personal_inbox_mutex, NULL);
    outbox = new_list();
    inbox = new_list();
    message_table = new_table();
    personal_inbox = new_list();

    printf("PEER ID: %s\nIP: %s\nVERSION: 0.0.1\n", conf.peer_id, conf.ip_address);

    /* create threads */
    pthread_create(&server_tid, NULL, net_server, NULL);
    if (conf.interface_port) {
        printf("INTERFACE IP: 127.0.0.1:%d\n", conf.interface_port);
        pthread_create(&interface_tid, NULL, remote_interface, NULL);
        pthread_join(interface_tid, NULL); /* if we have interface, wait on it */

    }else {

        pthread_join(server_tid, NULL); /* if no interface, wait on server */
    }

    return 0;
}
