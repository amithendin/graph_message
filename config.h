//
// Created by amit on 10/19/23.
//

#ifndef DISTMSG_CONFIG_H
#define DISTMSG_CONFIG_H

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "util.h"
#include "table.h"

typedef struct {
    char locale[50];
    char peer_id[PEER_ID_SIZE+1], *ip_address, host[BUFFER_SIZE];
    int port;
    Table *peer_table;
} Config;

int load_config(Config *conf, char *file_path);

int gen_config(Config *conf, char *file_path);

#endif //DISTMSG_CONFIG_H
