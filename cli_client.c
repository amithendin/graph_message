//
// Created by amit on 10/25/23.
//
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <time.h>
#include <pthread.h>

#define BUFFER_SIZE 1024
#define PEER_ID_SIZE 8

#define CMD_DISCOVER 1
#define CMD_SEND 2
#define CMD_CONNECT 3
#define CMD_FETCH_INBOX 4

typedef long long Time;

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

char engine_ip[BUFFER_SIZE];
int engine_port, client_socket;
pthread_mutex_t print_mutex;

char* serialize_command(Command cmd, size_t *len) {
    char *buff;

    *len = 1 + PEER_ID_SIZE + cmd.content_len;
    buff = malloc(*len);

    buff[0] = cmd.cmd;
    memcpy(buff+1, cmd.peer_id, PEER_ID_SIZE);
    if (cmd.content_len > 0) {
        memcpy(buff+1+PEER_ID_SIZE, cmd.content, cmd.content_len);
    }

    return buff;
}

ClientResponse *deserialize_response(char* buf, unsigned long long len) {
    ClientResponse *resp;

    resp = malloc(sizeof(ClientResponse));
    memcpy(&resp->time, buf, sizeof(Time));
    memcpy(resp->from_peer, buf+sizeof(Time), PEER_ID_SIZE);
    memcpy(&resp->content_len, buf+sizeof(Time)+PEER_ID_SIZE, sizeof(unsigned long long ));
    resp->content = malloc(resp->content_len);
    memcpy(resp->content, buf+sizeof(Time)+PEER_ID_SIZE+sizeof(unsigned long long), resp->content_len);

    return resp;
}

void miliseconds_to_datestr(Time t, char *buf) {
    time_t now;
    struct tm *timeinfo;
    int milliseconds;

    now = t/1000;
    milliseconds = t / 1000000000;
    timeinfo = localtime(&now);

    strftime(buf, 30, "%Y-%m-%d_%H:%M:%S", timeinfo);
    snprintf(buf + strlen(buf), 5, ".%d", milliseconds);
}

Command parse_command(char *input, int input_len, int *error) {
    Command cmd;
    int i;

    i=0;
    *error = 0;

    if (strncmp(input, "send", 4) == 0) {
        cmd.cmd = CMD_SEND;
        i += 4;
    }else if (strncmp(input, "discover", 8) == 0) {
        cmd.cmd = CMD_DISCOVER;
        cmd.content_len = 0;
        cmd.content = NULL;
        return cmd;
    }else if (strncmp(input, "connect", 7) == 0) {
        cmd.cmd = CMD_CONNECT;
        i += 7;
    }else if (strncmp(input, "fetch", 5) == 0) {
        cmd.cmd = CMD_FETCH_INBOX;
        cmd.content_len = 0;
        cmd.content = NULL;
        return cmd;
    }else {
        *error = 1;
        return cmd;
    }
    i += 1; /*for space*/

    memcpy(cmd.peer_id, input+i, PEER_ID_SIZE);

    i += PEER_ID_SIZE+1; /*for space*/

    cmd.content_len = input_len - i;
    if (cmd.content_len > 0) {
        cmd.content = malloc(cmd.content_len);
        memcpy(cmd.content, input + i, cmd.content_len);

    }else {
        *error = 1;
    }

    return cmd;
}

void handle_resp(ClientResponse *resp) {
    char buf[BUFFER_SIZE];

    miliseconds_to_datestr(resp->time, buf);

    pthread_mutex_lock(&print_mutex);
    printf("%.*s> (%s) %.*s\n", PEER_ID_SIZE, resp->from_peer, buf, resp->content_len, resp->content);
    pthread_mutex_unlock(&print_mutex);
}

void *reciever(void* varpg) {
    int read_size;
    char buffer[BUFFER_SIZE];
    size_t total_buff_len, total_buff_offset;
    char *total_buff;
    ClientResponse *resp;

    while(1) {
        while ((read_size = recv(client_socket, buffer, BUFFER_SIZE, 0)) > 0) {
            if (total_buff_len == 0) {
                memcpy(&total_buff_len, buffer, sizeof(size_t));
                if (total_buff_len == 0) {
                    break;
                }
                total_buff = malloc(total_buff_len);
                if (read_size > sizeof(size_t)) {
                    read_size -= sizeof(size_t);
                    memcpy(total_buff, buffer + sizeof(size_t), read_size);
                    total_buff_offset += read_size;
                }
            } else {
                memcpy(total_buff + total_buff_offset, buffer, read_size);
                total_buff_offset += read_size;
            }

            if (total_buff_offset >= total_buff_len - sizeof(size_t)) {
                break;
            }
        }

        if (total_buff_len > 0) {
            resp = deserialize_response(total_buff, total_buff_len);
            free(total_buff);
            total_buff_len = total_buff_offset = 0;

            handle_resp(resp);
            free(resp->content);
            free(resp);

        }else {

            sleep(1);
        }
    }

    return NULL;
}

void *sender(void *varpg) {
    int cmd_parse_err;
    struct sockaddr_in server_addr;
    char buffer[BUFFER_SIZE];
    size_t serialized_cmd_len, total_buff_len;
    char *serialized_cmd, *total_buff;
    Command cmd;

    // Create socket
    client_socket = socket(AF_INET, SOCK_STREAM, 0);

    // Initialize the server address structure
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(engine_port);
    server_addr.sin_addr.s_addr = inet_addr(engine_ip);

    // Connect to the server
    connect(client_socket, (struct sockaddr *)&server_addr, sizeof(server_addr));

    printf("CONNECTED TO %s:%d\n",engine_ip,engine_port);

    cmd.content_len = 0;
    while (1) {
        // Send data to the server
        pthread_mutex_lock(&print_mutex);
        printf("@>");
        pthread_mutex_unlock(&print_mutex);
        fgets(buffer, sizeof(buffer), stdin);

        if (strcmp(buffer, "exit\n") == 0) {
            break; // Exit the loop
        }

        cmd = parse_command(buffer, strlen(buffer), &cmd_parse_err);

        if (cmd_parse_err) {
            printf("invalid command\n");
            continue;
        }

        serialized_cmd = serialize_command(cmd, &serialized_cmd_len);
        if (cmd.content_len > 0) {
            cmd.content_len = 0;
            free(cmd.content);
        }

        total_buff_len = serialized_cmd_len+sizeof(size_t);
        total_buff = malloc(total_buff_len);
        memcpy(total_buff, &total_buff_len, sizeof(size_t));
        memcpy(total_buff+sizeof(size_t), serialized_cmd, serialized_cmd_len);
        free(serialized_cmd);

        send(client_socket, total_buff, total_buff_len, 0);

        free(total_buff);
        total_buff_len = 0;
    }

    close(client_socket);

    return NULL;
}

int main(int argc, char **argv) {
    pthread_t reciever_tid, sender_tid;
    char *tmp;
    int i;

    strcpy(engine_ip, "127.0.0.1");
    engine_port = 8080;
    tmp = NULL;
    if (argc > 1) {
        tmp = (char *) argv[1];
    } else {
        tmp = getenv("DISTMSG_INTERFACE_ADDR");
    }

    if (tmp != NULL) {
        for (i = 0; tmp[i] != 0; i++) {
            if (tmp[i] == ':') {
                tmp = tmp + i + 1;
                engine_ip[i] = 0;
                break;
            }
            engine_ip[i] = tmp[i];
        }
        engine_port = atoi(tmp);
    }

    pthread_mutex_init(&print_mutex, NULL);

    pthread_create(&reciever_tid, NULL, reciever, NULL);
    pthread_create(&sender_tid, NULL, sender, NULL);
    pthread_join(sender_tid, NULL); /* if we have interface, wait on it */

    return 0;
}