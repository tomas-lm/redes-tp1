#include "common.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#define BUFFER 1024

// Functions prototypes
void process_and_print_filtered_map(int *map, int size);
void print_possible_moves(int *possible_moves, int size);


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
    struct action message;
    memset(buf, 0, BUFFER);
    memset(&message, 0, sizeof(message));

    while (1) {
        // Solicita o comando do usuário
        printf("Digite o comando: ");
        fgets(buf, BUFFER - 1, stdin);
        buf[strcspn(buf, "\n")] = 0; // Remove o '\n' da entrada

        // Verifica se o comando é para sair
        if (strcmp(buf, "quit") == 0) {
            printf("Exiting...\n");
            break;
        }

        // Mapeia o comando para a struct action
        memset(&message, 0, sizeof(message));
        if (!map_command_to_action(buf, &message)) {
            printf("Comando inválido. Tente novamente.\n");
            continue;
        }

        // Envia a struct
        size_t count = send(s, &message, sizeof(message), 0);
        if (count != sizeof(message)) {
            perror("send error");
            continue;
        }

        struct action response;
        memset(&response, 0, sizeof(response));

        
        size_t total = 0;
        size_t expected_size = sizeof(response);

        while (total < expected_size) {
            printf("Receiving response...\n");
            size_t count = recv(s, &response, expected_size - total, 0);
            if (count <= 0) {
                printf("Error or server disconnected\n");
                break;
            }
            total += count;
        }
        
        // Print response.type
        printf("Response type: %d\n", response.type);

        //! =====================================
        //! LIDA COM A RESPOSTA VINDA DO SERVIDOR
        //! =====================================


        if (message.type == 0) //* START
        {
            printf("Jogo iniciado.\n");
        }
        else if (message.type == 1) //* MOVIMENTO
        {
            int possible_moves[4] = {0, 0, 0, 0};
            memcpy(possible_moves, buf, sizeof(possible_moves));
            print_possible_moves(possible_moves, 4);
        }
        else if (message.type == 2) //* MAPA
        {
        printf("Mapa recebido:\n");
        int *map = (int *)buf;
        int map_size = 10;
        process_and_print_filtered_map(map, map_size);
        }
        else if (message.type == 3) //* DICA
        {
            printf("Dica não foi implementado ainda\n");
        }
        else if (message.type == 4) //* UPDATE
        {
            printf("Update não foi implementado ainda\n");
        }
        else if (message.type == 5) //* WIN
        {
            printf("Você venceu!\n");
        }
        else if (message.type == 6) //* RESET
        {
            printf("Jogo resetado.\n");
        }
        else if (message.type == 7) //* EXIT
        {
            printf("Saindo...\n");
            break;
        }
        else 
        {
            printf("Resposta: %s\n", buf);
        }
    }

    close(s);
    return 0;
}




void process_and_print_filtered_map(int *map, int size) {
    char filtered_map[size][size];
    memset(filtered_map, 0, sizeof(filtered_map)); // Initialize to avoid garbage

    int filtered_rows = 0;
    for (int i = 0; i < size; i++) {
        int row_has_valid_data = 0;
        for (int j = 0; j < size; j++) {
            int value = map[i * size + j];

            // Map integers to character representations
            switch (value) {
                case 0:
                    filtered_map[filtered_rows][j] = '#'; // Muro
                    row_has_valid_data = 1;
                    break;
                case 1:
                    filtered_map[filtered_rows][j] = '_'; // Caminho livre
                    row_has_valid_data = 1;
                    break;
                case 2:
                    filtered_map[filtered_rows][j] = '>'; // Entrada
                    row_has_valid_data = 1;
                    break;
                case 3:
                    filtered_map[filtered_rows][j] = 'X'; // Saída
                    row_has_valid_data = 1;
                    break;
                case 4:
                    filtered_map[filtered_rows][j] = '?'; // Não descoberto
                    row_has_valid_data = 1;
                    break;
                case 5:
                    filtered_map[filtered_rows][j] = '+'; // Jogador
                    row_has_valid_data = 1;
                    break;
                default:
                    filtered_map[filtered_rows][j] = '\0'; // Ensure invalid values are skipped
                    break;
            }
        }

        // Only increment filtered_rows if the row has valid data
        if (row_has_valid_data) {
            filtered_rows++;
        }
    }

    // Print the filtered map
    printf("Mapa processado:\n");
    for (int i = 0; i < filtered_rows; i++) {
        for (int j = 0; j < size; j++) {
            if (filtered_map[i][j] != '\0') { // Skip empty cells
                printf("%c\t", filtered_map[i][j]);
            }
        }
        printf("\n");
    }
}



void print_possible_moves(int *possible_moves, int size) {
    char *directions[] = {"up", "right", "down", "left"};
    int first = 1; // Track if it's the first direction to avoid extra commas

    printf("Possible moves: ");
    for (int i = 0; i < size; i++) {
        if (possible_moves[i] == 0) break; // Stop at 0 (no more valid moves)
        if (!first) {
            printf(", ");
        }
        printf("%s", directions[possible_moves[i] - 1]);
        first = 0;
    }
    printf(".\n");
}