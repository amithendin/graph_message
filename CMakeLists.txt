cmake_minimum_required(VERSION 3.26)
project(distmsg C)

set(CMAKE_C_STANDARD 99)

add_executable(distmsg main.c
        table.h
        table.c
        list.h
        list.c
        message.c
        message.h
        util.h
        util.c
        config.h
        config.c
)
