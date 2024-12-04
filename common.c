#include "common.h"

#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>
#include <string.h>
#include <arpa/inet.h>

// Função para logar os erros que acontecerem no processo
void logexit(const char *str){
    if(str != NULL){
        perror(str);
    } else {
        perror("");
    }
    exit(EXIT_FAILURE);
}

int addrparse(const char *addrstr, const char *portstr, struct sockaddr_storage *storage)
{
    if (addrstr == NULL || portstr == NULL) {
        printf("[addrparse] Error: addrstr or portstr is NULL.\n");
        return -1;
    }

    printf("[addrparse] Parsing address: '%s', port: '%s'\n", addrstr, portstr);

    uint16_t port = (uint16_t)atoi(portstr); // Unsigned port
    if (port == 0) {
        printf("[addrparse] Error: Invalid port '%s'.\n", portstr);
        return -1;
    }
    port = htons(port); // Host to network short
    printf("[addrparse] Parsed port (network byte order): %d\n", port);

    struct in_addr inaddr4; // 32 bits IP Address
    //! IPV4
    if (inet_pton(AF_INET, addrstr, &inaddr4)) {
        printf("[addrparse] Detected IPv4 address: '%s'\n", addrstr);
        struct sockaddr_in *addr4 = (struct sockaddr_in *)storage;
        addr4->sin_family = AF_INET;
        addr4->sin_port = port;
        addr4->sin_addr = inaddr4;
        printf("[addrparse] IPv4 setup completed.\n");
        return 0;
    } else {
        printf("[addrparse] '%s' is not a valid IPv4 address.\n", addrstr);
    }

    struct in6_addr inaddr6; // 128 bits IP Address
    //! IPV6
    if (inet_pton(AF_INET6, addrstr, &inaddr6)) {
        printf("[addrparse] Detected IPv6 address: '%s'\n", addrstr);
        struct sockaddr_in6 *addr6 = (struct sockaddr_in6 *)storage;
        addr6->sin6_family = AF_INET6;
        addr6->sin6_port = port;
        memcpy(&(addr6->sin6_addr), &inaddr6, sizeof(inaddr6));
        printf("[addrparse] IPv6 setup completed.\n");
        return 0;
    } else {
        printf("[addrparse] '%s' is not a valid IPv6 address.\n", addrstr);
    }

    printf("[addrparse] Error: '%s' is not a valid IPv4 or IPv6 address.\n", addrstr);
    return -1;
}



void addrtostr(const struct sockaddr *addr, char *str, size_t strsize)
{
    int version;
    uint16_t port;
    char addrstr[INET6_ADDRSTRLEN + 1] = "";

    


    if(addr->sa_family == AF_INET){
        version = 4;
        struct sockaddr_in *addr4 = (struct sockaddr_in *)addr;
        if (!inet_ntop(AF_INET, &(addr4->sin_addr), addrstr, INET_ADDRSTRLEN + 1)){
            logexit("addrtostr: inet_ntop");
        }
        port = ntohs(addr4->sin_port);


    } else if(addr->sa_family == AF_INET6){
        version = 6;
        struct sockaddr_in6 *addr6 = (struct sockaddr_in6 *)addr;
        if (!inet_ntop(AF_INET6, &(addr6->sin6_addr), addrstr, INET6_ADDRSTRLEN + 1)){
            logexit("addrtostr: inet_ntop");
        }
        port = ntohs(addr6->sin6_port);
    } else {
        logexit("addrtostr: unknown address family");
    }
    if(str){
        snprintf(str, strsize, "IPV%d %s %hu", version, addrstr, port);
    }
}


int server_sockaddr_init(const char *proto, const char *portstr, struct sockaddr_storage *storage)
{
    uint16_t port = (uint16_t)atoi(portstr);
    if(port == 0){
        return -1;
    }
    port = htons(port);

    memset(storage, 0, sizeof(*storage));

    if(strcmp(proto, "v4") == 0){
        struct sockaddr_in *addr4 = (struct sockaddr_in *)storage;
        addr4->sin_family = AF_INET;
        addr4->sin_port = port;
        addr4->sin_addr.s_addr = INADDR_ANY;
        return 0;
    } else if(strcmp(proto, "v6") == 0){
        struct sockaddr_in6 *addr6 = (struct sockaddr_in6 *)storage;
        addr6->sin6_family = AF_INET6;
        addr6->sin6_port = port;
        addr6->sin6_addr = in6addr_any;
        return 0;
    } else {
        return -1;
    }
}



int map_command_to_action(const char *command, struct action *message) {
    if (strcmp(command, "start") == 0) {
        message->type = 0; // Comando "start"
        return 1;
    } else if (strcmp(command, "right") == 0 || strcmp(command, "left") == 0 || strcmp(command, "up") == 0 || strcmp(command, "down") == 0) {
        message->type = 1; // Comando "move"
        if (strcmp(command, "right") == 0) {
            message->moves[0] = 2;
        } else if (strcmp(command, "left") == 0) {
            message->moves[0] = 4;
        } else if (strcmp(command, "up") == 0) {
            message->moves[0] = 1;
        } else if (strcmp(command, "down") == 0) {
            message->moves[0] = 3;
        }
        return 1;
    } else if (strcmp(command, "map") == 0) {
        message->type = 2; // Comando "map"
        return 1;
    } else if (strcmp(command, "hint") == 0) {
        message->type = 3; // Comando "hint"
        return 1;
    } else if (strcmp(command, "reset") == 0) {
        message->type = 6; // Comando "reset"
        return 1;
    } else if (strcmp(command, "exit") == 0) {
        message->type = 7; // Comando "exit"
        return 1;
    } else {
        printf("Comando inválido.\n");
        return 0;
    }
}
