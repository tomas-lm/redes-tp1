#include "common.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#define BUFFER 1024

// Função para definir como é o uso do programa
void usage(int argc, char **argv){
    printf("usage: %s <server_ip> <server_port>\n", argv[0]);
    printf("example: %s 127.0.0.1 51511\n", argv[0]);
    exit(EXIT_FAILURE);
}


int main(int argc, char **argv){
    if(argc != 3){
        usage(argc, argv);
    }

    printf("argv[1] (address): %s\n", argv[1]);
    printf("argv[2] (port): %s\n", argv[2]);

    // Cria a estrutura de endereço
    struct sockaddr_storage storage;
    if (0 != addrparse(argv[1], argv[2], &storage)) {
        usage(argc, argv);
    }


    // Cria o scoket
    int s;
    s = socket(storage.ss_family, SOCK_STREAM, 0);
    if(s < 0){
        logexit("socket error");
    }


    // Interface de endereço
    struct sockaddr *addr = (struct sockaddr *)(&storage);

    if (0 != connect(s, addr, 
                 (addr->sa_family == AF_INET) ? sizeof(struct sockaddr_in) : 
                 sizeof(struct sockaddr_in6))) {
    logexit("connect error");   
    }


    char addrstr[BUFFER];
    addrtostr(addr, addrstr, BUFFER);


    printf("connected to %s\n", addrstr);

    // Inicializa buffer
    char buf[BUFFER];
    // Inicializa valores do buffer e 0
    memset(buf, 0, BUFFER);


    // Mensagem
    printf("Digite a mensagem: ");
    fgets(buf, BUFFER - 1, stdin);
    buf[strcspn(buf, "\n")] = 0;

    // Salva comando na mensagem
    struct action message;
    memset(&message, 0, sizeof(message));



    // Mapeia o comando para a struct action
    if (!map_command_to_action(buf, &message)) {
        logexit("Comando inexistente");
    }

    // Print message
    printf("message: %d\n", message.type);

    // Envia a struct
    size_t count = send(s, &message, sizeof(message), 0);
    if (count != sizeof(message)) {
        logexit("send error");
    }

    printf("sent %zu bytes\n", count);

    if(count != sizeof(message)){
        logexit("send error");
    }

    // Reseta o buffer
    memset(buf, 0, BUFFER);

    unsigned total = 0;
    // Enquanto a comunicação estiver rolando
    while(1){

        count = recv(s, buf + total, BUFFER - total, 0);
        if(count == 0){
            break;
        }
        total += count;

    }

    printf("received %u bytes\n", total);
    printf("received message: %s\n", buf);
    puts(buf);

    close(s);
}