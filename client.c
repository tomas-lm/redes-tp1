#include "common.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#define BUFFER 1024

// Global vars
int moves[4] = {0, 0, 0, 0};
int game_start = 0;

// Functions prototypes
void process_and_print_filtered_map(int board[10][10], int size);
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

    while (1) 
    {
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
            printf("error: command not found\n");
            continue;
        }
        //! =====================================
        //! TRATANDO O COMANDO DO USUARIO
        //! =====================================
        if(message.type == 0 && game_start == 0){
            game_start = 1;
        } 
        else if (message.type !=0 && game_start == 0){
            printf("error: start the game first\n");
            continue;
        } 

        if(message.type == 1){
            // Verifica se o movimento é válido
            int valid = 0;
            for (int i = 0; i < 4; i++) {
                if (moves[i] == message.moves[0]) {
                    valid = 1;
                    break;
                }
            }
            if (!valid) {
                printf("error: you cannot go this way\n");
                continue;
            }
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
            size_t count = recv(s, &response, expected_size, 0);
            if (count <= 0) {
                printf("Error or server disconnected\n");
                break;
            }
            total += count;
        }


        //! =====================================
        //! LIDA COM A RESPOSTA VINDA DO SERVIDOR
        //! =====================================

        if (message.type == 7) //* EXIT
        {
            printf("Saindo...\n");
            break;
        }

        if (response.type == 4) // UPDATE
        {
            if (message.type == 0) //* START
            {
                print_possible_moves(response.moves, 4);
                // update moves
                memcpy(moves, response.moves, sizeof(moves));
            }
            else if (message.type == 1) //* MOVIMENTO
            {
                int possible_moves[4] = {0, 0, 0, 0};
                memcpy(possible_moves, response.moves, sizeof(possible_moves));
                print_possible_moves(possible_moves, 4);
                // update moves
                memcpy(moves, response.moves, sizeof(moves));
            }
            else if (message.type == 2) //* MAPA
            {
            process_and_print_filtered_map(response.board, 10);
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
                print_possible_moves(response.moves, 4);
                // update moves
                memcpy(moves, response.moves, sizeof(moves));
            }
            else 
            {
                printf("Resposta: %s\n", buf);
            }
        }
        else if (response.type == 5) // WIN
        {
            printf("Você venceu! Apenas use os comando reset ou exit\n");
            process_and_print_filtered_map(response.board, 10);
        }
        else 
        {
            printf("Resposta: %s\n", buf);
        }

    }
    printf("Fechando o socket\n");
    close(s);
    return 0;
}

void process_and_print_filtered_map(int board[10][10], int size) {

    // Determine the bounds for valid rows and columns
    int border = 0; // Adjust to find the actual valid size
    for (int i = 0; i < size; i++) {
        for (int j = 0; j < size; j++) {
            if (board[i][j] != -1) {
                if (i + 1 > border) border = i + 1; // Update to include valid rows
                if (j + 1 > border) border = j + 1; // Update to include valid columns
            }
        }
    }

    printf("Border size: %d\n", border);
    // Validate that there's no padding (-1) in the filtered map
    if (border == 0) {
        return; // Exit early if all padding
    }

    // Print the filtered map with valid dimensions
    for (int i = 0; i < border; i++) {
        for (int j = 0; j < border; j++) {
            int value = board[i][j];

            // Map integers to character representations
            char symbol = ' ';
            switch (value) {
                case 0: symbol = '#'; break; // Wall
                case 1: symbol = '_'; break; // Free path
                case 2: symbol = '>'; break; // Entrance
                case 3: symbol = 'X'; break; // Exit
                case 4: symbol = '?'; break; // Undiscovered
                case 5: symbol = '+'; break; // Player
            }

            printf("%c\t", symbol); // Print with tab spacing
        }
        printf("\n"); // New line at the end of each row
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