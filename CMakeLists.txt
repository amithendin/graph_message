cmake_minimum_required(VERSION 3.26)
project(distmsg C)

set(CMAKE_C_STANDARD 99)

add_executable(distmsg main.c
        table.c
        list.c
        message.c
        util.c
        config.c
)

add_executable(client cli_client.c)
