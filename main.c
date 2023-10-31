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

#include "util.h"
#include "table.h"
#include "list.h"
#include "message.h"
#include "config.h"

#define CMD_DISCOVER 1
#define CMD_SEND 2
#define CMD_CONNECT 3
#define CMD_FETCH_INBOX 4

typedef struct {
    char cmd;
    char peer_id[PEER_ID_SIZE];
    unsigned long long content_len;
    char *content;
} Command;

typedef struct {
    Time time;
    char from_peer[PEER_ID_SIZE];
    unsigned long long content_len;
    char *content;
} ClientResponse;

int client_socket;
List *outbox, *inbox, *personal_inbox;
Table *message_table;
pthread_mutex_t outbox_mutex, inbox_mutex, message_table_mutex, personal_inbox_mutex;
Config conf;

void server();
void client();

Command deserialize_command(char *buff, size_t buff_len) {
    Command cmd;

    cmd.cmd = buff[0];
    buff_len -= 1;
    memcpy(cmd.peer_id, buff+1, PEER_ID_SIZE);
    buff_len -= PEER_ID_SIZE;
    cmd.content_len = buff_len;
    if (buff_len > 0) {
        cmd.content = malloc(buff_len);
        memcpy(cmd.content, buff + 1 + PEER_ID_SIZE, buff_len);
    }

    return cmd;
}

Buffer serialize_response(ClientResponse *resp) {
    Buffer buf;

    buf.len = sizeof(Time) + PEER_ID_SIZE + sizeof(unsigned long long) + resp->content_len;
    buf.data = malloc(buf.len);

    memcpy(buf.data, &resp->time, sizeof(Time));
    memcpy(buf.data+sizeof(Time), resp->from_peer, PEER_ID_SIZE);
    memcpy(buf.data+sizeof(Time)+PEER_ID_SIZE, &resp->content_len, sizeof(unsigned long long ));
    memcpy(buf.data+sizeof(Time)+PEER_ID_SIZE+sizeof(unsigned long long ), resp->content, resp->content_len);

    return buf;
}

void enqueue_message(pthread_mutex_t *queue_mutex, List *queue, Message *msg) {
    pthread_mutex_lock(queue_mutex);
    list_push(queue, msg);
    pthread_mutex_unlock(queue_mutex);
}

Message* dequeue_message(pthread_mutex_t *queue_mutex, List *queue) {
    Message *msg;

    pthread_mutex_lock(queue_mutex);
    msg = (Message *) list_poplast(queue);
    pthread_mutex_unlock(queue_mutex);

    return msg;
}

int net_client(char *addr, int port, Message *msg) {
    int sock;
    struct sockaddr_in server;
    Buffer *buff;

    buff = serialize_msg(msg);

    //Create socket
    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock == -1) {
        printf("failed to create socket\n");
    }

    server.sin_addr.s_addr = inet_addr(addr);
    server.sin_family = AF_INET;
    server.sin_port = htons(port);

    //Connect to remote net_server
    if (connect(sock, (struct sockaddr *) &server, sizeof(server)) < 0) {
        perror("connect failed\n");
        free(buff);
        return 1;
    }

    //keep communicating with net_server
    if (send(sock, buff->data, buff->len, 0) < 0) {
        printf("send failed\n");
        free(buff);
        return 1;
    }

    close(sock);

    free(buff);
    return 0;
}

void *net_server(void *vargp) {
    int listenfd, client_sock;
    long read_size;
    struct sockaddr_in serv_addr;
    char buff[BUFFER_SIZE];
    Buffer *tmp, *total_buff;
    List *buff_chain;
    Message *msg;

    listenfd = socket(AF_INET, SOCK_STREAM, 0);

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    serv_addr.sin_port = htons(conf.port);

    bind(listenfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr));

    if (listen(listenfd, 10) == -1) {
        printf("failed to listen\n");
        return NULL;
    }

    while (1) {
        client_sock = accept(listenfd, (struct sockaddr *) NULL, NULL); // accept awaiting request
        if (client_sock < 0) {
            perror("accept failed\n");
            return NULL;
        }

        total_buff = (Buffer *) malloc(sizeof(Buffer));
        total_buff->len = 0;
        buff_chain = new_list();
        while ((read_size = recv(client_sock, buff, BUFFER_SIZE, 0)) > 0) {
            tmp = malloc(sizeof (Buffer));
            tmp->len = read_size;
            tmp->data = malloc(read_size);
            memcpy(tmp->data, buff, read_size);
            list_push(buff_chain, tmp);
            total_buff->len += read_size;
        }
        total_buff->data = consume_buff_chain(buff_chain, total_buff->len);
        msg = deserialize_msg(total_buff->data);

        enqueue_message(&inbox_mutex, inbox, msg);

        server();

        free_buffer(total_buff);
        memset(buff, 0, BUFFER_SIZE);
        close(client_sock);
    }

    return NULL;
}

void client() {
    int i, port;
    char addr[BUFFER_SIZE], *tmp;
    Message *msg;
    Buffer peer_req_data, *tmp_buf, tmp_peer_id;

    tmp = NULL;
    msg = NULL;
    peer_req_data.len = 0;
    peer_req_data.data = NULL;

    msg = dequeue_message(&outbox_mutex, outbox);

    if (msg != NULL) {
        if (msg->through_peer[0]) {
            tmp_peer_id = buffer_from_str(msg->through_peer, PEER_ID_SIZE);
            tmp_buf = table_search(conf.peer_table, tmp_peer_id);
        }else {
            tmp_peer_id = buffer_from_str(msg->to_peer, PEER_ID_SIZE);
            tmp_buf = table_search(conf.peer_table, tmp_peer_id);
        }

        if (tmp_buf != NULL) {
            tmp = (char*) tmp_buf->data;
            for (i = 0; tmp[i] != 0; i++) {
                if (tmp[i] == ':') {
                    tmp = tmp + i + 1;
                    addr[i] = 0;
                    break;
                }
                addr[i] = tmp[i];
            }
            port = atoi(tmp);
            net_client(addr, port, msg);
            free_message(msg);

        } else {
            enqueue_message(&inbox_mutex, inbox, msg);
            server();
        }

    }
}

void server() {
    Message *msg, *discover_msg, *broadcast_msg;
    TableIter *it;
    DeserializeTableIter *de_it;
    Buffer *table_buf;
    Buffer sgn, *lookup, new_buf;
    char *time_str;
    ClientResponse *resp;

    msg = dequeue_message(&inbox_mutex, inbox);

    if (msg != NULL) {
        if (strncmp(msg->from_peer, "discover", PEER_ID_SIZE) == 0) {
            de_it = deserialize_table_iter(&msg->content);

            while (deserialize_table_iter_next(de_it)) {
                table_insert(conf.peer_table, de_it->curr->key, de_it->curr->value);
            }

            free(de_it);
            de_it = NULL;
            free_message(msg);

        }else if (strncmp(msg->to_peer, "discover", PEER_ID_SIZE) == 0) {
            table_buf = serialize_table(conf.peer_table);
            discover_msg = new_message(table_buf, "discover", msg->from_peer);

            enqueue_message(&outbox_mutex, outbox, discover_msg);

            client();

            free_buffer(table_buf);
            free_message(msg);

        } else {

            sgn = gen_message_signature(msg);
            lookup = table_search(message_table, sgn);

            if (lookup == NULL) {
                new_buf.data = NULL;
                new_buf.len = 0;
                table_insert(message_table, sgn, new_buf);

                if (strncmp(msg->to_peer, conf.peer_id, PEER_ID_SIZE) == 0) {

                    resp = malloc(sizeof(ClientResponse));
                    resp->time = msg->time;
                    memcpy(resp->from_peer, msg->from_peer, PEER_ID_SIZE);
                    resp->content_len = msg->content.len;
                    resp->content = malloc(resp->content_len);
                    memcpy(resp->content, msg->content.data, resp->content_len);

                    if (conf.interface_port) {
                        pthread_mutex_lock(&personal_inbox_mutex);
                        list_push(personal_inbox, resp);
                        pthread_mutex_unlock(&personal_inbox_mutex);
                    }else {
                        time_str = miliseconds_to_datestr(msg->time);
                        printf("%.*s> [%s] \"%s\"\n", PEER_ID_SIZE, msg->from_peer, time_str, (char *) msg->content.data);
                        free(time_str);
                    }
                } else {

                    it = table_iter_new(conf.peer_table);

                    while (table_iter_next(it)) {
                        if (strncmp((char*)it->curr->key.data, conf.peer_id, PEER_ID_SIZE) != 0 &&
                            strncmp((char*)it->curr->key.data, msg->from_peer, PEER_ID_SIZE) != 0) {
                            broadcast_msg = new_message(&msg->content, msg->from_peer, msg->to_peer);
                            memcpy(broadcast_msg->through_peer, it->curr->key.data, PEER_ID_SIZE);
                            broadcast_msg->time = msg->time;
                            enqueue_message(&outbox_mutex, outbox, broadcast_msg);
                        }
                    }
                    client();

                    free(it);
                    it = NULL;
                }
            }

            free_message(msg);
            free(sgn.data);
        }
    }
}

void execute_command(Command cmd) {
    char peer_id[PEER_ID_SIZE+1];
    Message *msg, *discover_msg;
    Buffer tmp, *table_buf;
    TableIter *it;

    if (cmd.cmd == CMD_CONNECT) {
        table_buf = serialize_table(conf.peer_table);
        table_insert(conf.peer_table, buffer_from_str(cmd.peer_id, 0), buffer_from_str(cmd.content, 1));

        discover_msg = new_message(table_buf, "discover", peer_id);
        free_buffer(table_buf);

        enqueue_message(&outbox_mutex, outbox, discover_msg);
        client();

    }else if (cmd.cmd == CMD_DISCOVER) {
        tmp = buffer_from_str("0", 1);

        it = table_iter_new(conf.peer_table);

        while (table_iter_next(it)) {
            if (strncmp((char*)it->curr->key.data, conf.peer_id, PEER_ID_SIZE) != 0) {
                discover_msg = new_message(&tmp, conf.peer_id, "discover");
                memcpy(discover_msg->through_peer, it->curr->key.data, PEER_ID_SIZE);
                enqueue_message(&outbox_mutex, outbox, discover_msg);
            }
        }
        client();

        free(tmp.data);
        free(it);
        it = NULL;

    }else if (cmd.cmd == CMD_SEND) {
        tmp.len = cmd.content_len;
        tmp.data = cmd.content;
        msg = new_message(&tmp, conf.peer_id, cmd.peer_id);
        enqueue_message(&outbox_mutex, outbox, msg);
        client();
    }

}

void *remote_interface(void *varpg) {
    int server_socket;
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_addr_len = sizeof(client_addr);
    char buff[BUFFER_SIZE], *total_buff, *tmp;
    size_t total_buff_len, total_buff_offset, buff_len;
    Command cmd;
    ClientResponse *resp;
    Buffer serialized_resp;

    // Create socket
    server_socket = socket(AF_INET, SOCK_STREAM, 0);

    // Initialize the server address structure
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(conf.interface_port);
    server_addr.sin_addr.s_addr = INADDR_ANY;

    // Bind the socket to the server address
    bind(server_socket, (struct sockaddr *)&server_addr, sizeof(server_addr));

    // Listen for incoming connections
    listen(server_socket, 5);

    while (1) {
        // Accept a connection
        client_socket = accept(server_socket, (struct sockaddr *)&client_addr, &client_addr_len);

        while (1) {
            // Receive data from the client
            total_buff_len = total_buff_offset = 0;

            while ((buff_len = recv(client_socket, buff, BUFFER_SIZE, 0)) > 0) {
                if (total_buff_len == 0) {
                    memcpy(&total_buff_len, buff, sizeof(size_t));
                    total_buff = malloc(total_buff_len);
                    if (buff_len > sizeof(size_t)) {
                        buff_len -= sizeof(size_t);
                        memcpy(total_buff, buff + sizeof(size_t), buff_len);
                        total_buff_offset += buff_len;
                    }
                }else {
                    memcpy(total_buff+total_buff_offset, buff, buff_len);
                    total_buff_offset += buff_len;
                }

                if (total_buff_offset >= total_buff_len - sizeof(size_t)) {
                    cmd = deserialize_command(total_buff, total_buff_offset);
                    free(total_buff);
                    total_buff_len = total_buff_offset = 0;

                    execute_command(cmd);

                    if (cmd.content_len > 0) {
                        free(cmd.content);
                        cmd.content_len = 0;
                    }

                    tmp = "command executed";
                    resp = malloc(sizeof(ClientResponse));
                    resp->time = now_milliseconds();
                    memcpy(resp->from_peer, "PEERCLNT", PEER_ID_SIZE);
                    resp->content_len = strlen(tmp);
                    resp->content = tmp;
                    serialized_resp = serialize_response(resp);
                    free(resp);

                    total_buff_len = serialized_resp.len+ sizeof(size_t);
                    total_buff = malloc(total_buff_len);
                    memcpy(total_buff, &total_buff_len, sizeof(size_t));
                    memcpy(total_buff + sizeof(size_t), serialized_resp.data, total_buff_len - sizeof(size_t));

                    send(client_socket, total_buff, total_buff_len, 0);
                    free(total_buff);
                    total_buff_len = total_buff_offset = 0;
                }
            }

            do {
                pthread_mutex_lock(&personal_inbox_mutex);
                resp = (ClientResponse *)list_poplast(personal_inbox);
                pthread_mutex_unlock(&personal_inbox_mutex);

                if (resp != NULL) {
                    serialized_resp = serialize_response(resp);
                    free(resp->content);
                    free(resp);

                    total_buff_len = serialized_resp.len + sizeof(size_t);
                    total_buff = malloc(total_buff_len);
                    memcpy(total_buff, &total_buff_len, sizeof(size_t));
                    memcpy(total_buff + sizeof(size_t), serialized_resp.data, total_buff_len - sizeof(size_t));
                    free(serialized_resp.data);

                    send(client_socket, total_buff, total_buff_len, 0);
                    free(total_buff);
                    total_buff_len = total_buff_offset = 0;
                }
            }while(resp != NULL);

            sleep(1);
        }

        close(client_socket);
    }
    close(server_socket);

    return 0;
}

int main(int argc, char *argv[]) {
    pthread_t server_tid, engine_tid, interface_tid;

    srand ( time(NULL) );

    if ( !load_config(&conf,argv[1]) ) {
        printf("CANNOT START WITHOUT VALID CONFIG FILE\n");
        return 1;
    }

    setlocale(LC_TIME, conf.locale);  // Use "en_US.utf8" for US English

    pthread_mutex_init(&outbox_mutex, NULL);
    pthread_mutex_init(&inbox_mutex, NULL);
    pthread_mutex_init(&message_table_mutex, NULL);
    pthread_mutex_init(&personal_inbox_mutex, NULL);
    outbox = new_list();
    inbox = new_list();
    message_table = new_table();
    personal_inbox = new_list();

    printf("PEER ID: %s\nIP: %s\nVERSION: 0.0.1\n", conf.peer_id, conf.ip_address);

    pthread_create(&server_tid, NULL, net_server, NULL);
    //pthread_create(&engine_tid, NULL, engine, NULL);
    if (conf.interface_port) {
        printf("INTERFACE IP: 127.0.0.1:%d\n", conf.interface_port);
        pthread_create(&interface_tid, NULL, remote_interface, NULL);
        pthread_join(interface_tid, NULL);

    }else {

        pthread_join(server_tid, NULL);
    }


    return 0;
}
