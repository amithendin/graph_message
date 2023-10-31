//
// Created by amit on 10/19/23.
//
#include "config.h"

int load_config(Config *conf,char *file_path) {
    FILE *fp;
    char *line, *key, *val, *tmp;
    short peer_table_mode, has_interface;
    ssize_t len, line_len, i, num_keys;

    (*conf).peer_table = new_table();

    num_keys = len = 0;
    peer_table_mode = has_interface = 0;
    line = NULL;
    fp = fopen(file_path, "r");

    if (fp == NULL) {
        if (!gen_config(conf, file_path)) {
            return 0;
        }
        num_keys = 4;

    }else {

        while ((line_len = getline(&line, &len, fp)) != -1) {
            i = 0;
            key = line;
            for (i = 0, key = line; line[i] != '=' && line[i] != '\0'; i++);
            line[i] = '\0';
            for (val = line + i + 1; line[i] != '\n' && line[i] != '\0'; i++);
            line[i] = '\0';

            if (val[strlen(val)-1]=='\n')
                val[strlen(val)-1] = '\0';

            if (peer_table_mode == 1) {
                //printf("INSERT PEER TABLE %s %s\n", key, val);
                table_insert((*conf).peer_table,
                             buffer_from_str(key, 0), buffer_from_str(val, -1));

            }else {

                if (strcmp(key, "host") == 0) {
                    strcpy((*conf).host, val);
                    num_keys++;
                } else if (strcmp(key, "port") == 0) {
                    (*conf).port = atoi(val);
                    num_keys++;
                } else if (strcmp(key, "peer_id") == 0) {
                    strcpy((*conf).peer_id, val);
                    num_keys++;
                } else if (strncmp(key, "locale",6) == 0) {
                    strcpy((*conf).locale, val);
                    num_keys++;

                } else if (strncmp(key, "interface_port",14) == 0) {
                    (*conf).interface_port = atoi(val);
                    has_interface = 1;

                } else if (strncmp(key, "peer_table",10) == 0) {

                    peer_table_mode = 1;
                }
            }
        }

        fclose(fp);
        if (line) {
            free(line);
        }
    }

    if (num_keys < 4) {
        if (!gen_config(conf, file_path)) {
            return 0;
        }
    }

    if (!has_interface) {
        conf->interface_port = 0;
    }

    (*conf).ip_address = malloc(BUFFER_SIZE);
    sprintf((*conf).ip_address, "%s:%d", (*conf).host, (*conf).port);
    table_insert((*conf).peer_table, buffer_from_str((*conf).peer_id, 0), buffer_from_str((*conf).ip_address,0));

    return 1;
}

int gen_config(Config *conf,char *file_path) {
    char buff[BUFFER_SIZE], *tmp;
    FILE *fp = fopen (file_path, "w");

    if (fp == NULL) {
        printf("UNABLE TO ACCESS FILE \"%s\"\n", file_path);
        return 0;
    }

    printf("FAILED TO OPEN CONFIGURATION FILE \"%s\" GENERATING IT INSTEAD.\n", file_path);
    printf("ENTER PEER_ID OR LEAVE BLANK TO GENERATE:");
    fgets(buff, BUFFER_SIZE, stdin);
    buff[strlen(buff) - 1] = 0;

    if (strlen(buff) == 0) {
        tmp = gen_peer_id();
        strcpy((*conf).peer_id, tmp);
        free(tmp);
    }else {
        strcpy((*conf).peer_id, buff);
    }
    fprintf (fp, "peer_id=%s\n", (*conf).peer_id);

    printf("ENTER HOST IP/NAME, LEAVE BLANK FOR LOCALHOST:");
    fgets(buff, BUFFER_SIZE, stdin);
    buff[strlen(buff) - 1] = 0;

    if (strlen(buff) == 0) {
        strcpy((*conf).host, "127.0.0.1");
    }else {
        strcpy((*conf).host, buff);
    }
    fprintf (fp, "host=%s\n", (*conf).host);

    printf("ENTER PORT NUMBER, LEAVE BLANK FOR DEFAULT PORT:");
    fgets(buff, BUFFER_SIZE, stdin);
    buff[strlen(buff) - 1] = 0;

    if (strlen(buff) == 0) {
        (*conf).port = 3000;
    }else {
        (*conf).port = atoi(buff);
    }
    fprintf (fp, "port=%d\n", (*conf).port);

    printf("ENTER LOCALE CODE, LEAVE BLANK FOR DEFAULT:");
    fgets(buff, BUFFER_SIZE, stdin);
    buff[strlen(buff) - 1] = 0;

    if (strlen(buff) == 0) {
        strcpy((*conf).locale, "en_IL.utf8");
    }else {
        strcpy((*conf).locale, buff);
    }
    fprintf (fp, "locale=%s\n", (*conf).locale);

    (*conf).interface_port = 8080;
    fprintf (fp, "interface_port=%d\n", (*conf).interface_port);

    fclose(fp);

    return 1;
}
