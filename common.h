#ifndef COMMON_H
#define COMMON_H
#pragma once
#include <stdlib.h>
#include <arpa/inet.h>

// RUN ./server v4 51511  
// ./client 127.0.0.1 51511    
struct action {
    int type;         
    int moves[100];   
    int board[10][10];
};

int map_command_to_action(const char *command, struct action *message);

void logexit(const char *str);
int addrparse(const char *addrstr, const char *portstr, struct sockaddr_storage *storage);
void addrtostr(const struct sockaddr *addr, char *str, size_t strsize);
int server_sockaddr_init(const char *proto, const char *portstr, struct sockaddr_storage *storage);


#endif