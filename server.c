#include "common.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#define BUFFER 1024
#define MAX_MAP_SIZE 10

// Global variables
int map[MAX_MAP_SIZE][MAX_MAP_SIZE];
int discovered_map[MAX_MAP_SIZE][MAX_MAP_SIZE];
int map_rows = 0, map_cols = 0;
int player_x = -1, player_y = -1;
int game_started = 0;


// Function prototypes
void load_map(const char *filename);
void usage(int argc, char **argv);
void handle_message(int csock, struct action *msg, const char *input_file);
void reset_game(const char *input_file);
void init_discovered_map();
void update_discovered_map();
void print_map(int discovered_map[MAX_MAP_SIZE][MAX_MAP_SIZE]);



// Reload the game state
void reset_game(const char *input_file) {
    load_map(input_file);      
    init_discovered_map();     
    player_x = player_y = -1;  
    game_started = 0;         
}

void init_discovered_map() {
    for (int i = 0; i < MAX_MAP_SIZE; i++) {
        for (int j = 0; j < MAX_MAP_SIZE; j++) {
            discovered_map[i][j] = 4;
        }
    }
}



void update_discovered_map() {
    discovered_map[player_x][player_y] = map[player_x][player_y];
    if (player_x > 0) discovered_map[player_x - 1][player_y] = map[player_x - 1][player_y]; // Up
    if (player_x < map_rows - 1) discovered_map[player_x + 1][player_y] = map[player_x + 1][player_y]; // Down
    if (player_y > 0) discovered_map[player_x][player_y - 1] = map[player_x][player_y - 1]; // Left
    if (player_y < map_cols - 1) discovered_map[player_x][player_y + 1] = map[player_x][player_y + 1]; // Right
}


void print_map(int discovered_map[MAX_MAP_SIZE][MAX_MAP_SIZE]) {
    printf("Complete Map:\n");
    for (int i = 0; i < map_rows; i++) {
        for (int j = 0; j < map_cols; j++) {
            printf("%d ", map[i][j]); // Full map from the server
        }
        printf("\n");
    }

    printf("\nDiscovered Map:\n");
    for (int i = 0; i < map_rows; i++) {
        for (int j = 0; j < map_cols; j++) {
            // Print the discovered map: use '?' (4) for undiscovered cells
            if (discovered_map[i][j] == 4) {
                printf("? ");
            } else {
                printf("%d ", discovered_map[i][j]);
            }
        }
        printf("\n");
    }
    printf("\n");
}


// Function to load the map from a file
void load_map(const char *filename) {
    FILE *file = fopen(filename, "r");
    if (!file) {
        perror("Failed to open map file");
        exit(EXIT_FAILURE);
    }

    char line[BUFFER];
    int row = 0;
    while (fgets(line, sizeof(line), file) && row < MAX_MAP_SIZE) {
        char *token = strtok(line, " ");
        int col = 0;
        while (token && col < MAX_MAP_SIZE) {
            map[row][col++] = atoi(token);
            token = strtok(NULL, " ");
        }
        if (map_cols == 0) {
            map_cols = col; // Set the number of columns based on the first row
        } else if (col != map_cols) {
            fprintf(stderr, "Inconsistent map dimensions\n");
            exit(EXIT_FAILURE);
        }
        row++;
    }
    map_rows = row;
    fclose(file);

    if (map_rows == 0 || map_cols == 0) {
        fprintf(stderr, "Map file is empty or invalid\n");
        exit(EXIT_FAILURE);
    }

    printf("Map loaded successfully (%dx%d)\n", map_rows, map_cols);
}

// Usage function
void usage(int argc, char **argv) {
    printf("usage: %s <v4|v6> <server_port> -i <input_file>\n", argv[0]);
    printf("example: %s v4 51511 -i input/in.txt\n", argv[0]);
    exit(EXIT_FAILURE);
}

void handle_message(int csock, struct action *msg, const char *input_file) {
    char buffer[BUFFER];
    memset(buffer, 0, BUFFER);

    switch (msg->type) {
        case 0: // start
            if (!game_started) {
                // Locate the entrance
                int found = 0;
                for (int i = 0; i < map_rows; i++) {
                    for (int j = 0; j < map_cols; j++) {
                        if (map[i][j] == 2) { // Found entrance
                            player_x = i;
                            player_y = j;
                            found = 1;
                            break;
                        }
                    }
                    if (found) break;
                }

                if (!found) {
                    snprintf(buffer, BUFFER, "error: no entrance (2) found on the map");
                    send(csock, buffer, strlen(buffer), 0);
                    return;
                }

                update_discovered_map();
                game_started = 1;
                snprintf(buffer, BUFFER, "Game started. Player at entrance (%d, %d).", player_x, player_y);
            } else {
                snprintf(buffer, BUFFER, "Game already started.");
            }
            break;

        case 1: // move
            if (!game_started) {
                snprintf(buffer, BUFFER, "error: start the game first");
            } else {
                int move = msg->moves[0];
                int valid_move = 0;
                if (move == 1 && player_x > 0 && map[player_x - 1][player_y] != 0) {
                    player_x--; // Up
                    valid_move = 1;
                } else if (move == 2 && player_y < map_cols - 1 && map[player_x][player_y + 1] != 0) {
                    player_y++; // Right
                    valid_move = 1;
                } else if (move == 3 && player_x < map_rows - 1 && map[player_x + 1][player_y] != 0) {
                    player_x++; // Down
                    valid_move = 1;
                } else if (move == 4 && player_y > 0 && map[player_x][player_y - 1] != 0) {
                    player_y--; // Left
                    valid_move = 1;
                }

                if (!valid_move) {
                    snprintf(buffer, BUFFER, "error: you cannot go this way");
                } else {
                    update_discovered_map();
                    snprintf(buffer, BUFFER, "Player moved to (%d, %d).", player_x, player_y);
                }
            }
            break;

        case 2: // map
            if (!game_started) {
                snprintf(buffer, BUFFER, "error: start the game first");
            } else {
                snprintf(buffer, BUFFER, "Partial map sent to client.");
                // Send the discovered map to the client
                send(csock, discovered_map, sizeof(discovered_map), 0);
                return;
            }
            break;

        case 3: // hint
            if (!game_started) {
                snprintf(buffer, BUFFER, "error: start the game first");
            } else {
                snprintf(buffer, BUFFER, "Hint: right, right, down."); // Example placeholder
            }
            break;

        case 6: // reset
            reset_game(input_file);
            snprintf(buffer, BUFFER, "Game reset.");
            break;

        case 7: // exit
            snprintf(buffer, BUFFER, "Goodbye!");
            send(csock, buffer, strlen(buffer), 0);
            close(csock);
            return;

        default: // Invalid command
            snprintf(buffer, BUFFER, "error: command not found");
    }

    send(csock, buffer, strlen(buffer), 0);
}


int main(int argc, char **argv) {
    if (argc != 5 || strcmp(argv[3], "-i") != 0) {
        usage(argc, argv);
    }

    const char *input_file = argv[4];
    reset_game(input_file); // Initialize game

    struct sockaddr_storage storage;
    if (server_sockaddr_init(argv[1], argv[2], &storage) != 0) {
        usage(argc, argv);
    }

    int s = socket(storage.ss_family, SOCK_STREAM, 0);
    if (s < 0) {
        logexit("socket error");
    }

    struct sockaddr *addr = (struct sockaddr *)(&storage);
    if (bind(s, addr, (addr->sa_family == AF_INET) ? sizeof(struct sockaddr_in) : sizeof(struct sockaddr_in6)) != 0) {
        logexit("bind error");
    }

    if (listen(s, 10) != 0) {
        logexit("listen error");
    }

    char addrstr[BUFFER];
    addrtostr(addr, addrstr, BUFFER);
    printf("bound to %s, waiting connection\n", addrstr);

    while (1) {
        struct sockaddr_storage cstorage;
        struct sockaddr *caddr = (struct sockaddr *)(&cstorage);
        socklen_t caddrlen = sizeof(cstorage);
        int csock = accept(s, caddr, &caddrlen);

        if (csock < 0) {
            logexit("accept error");
        }

        char caddrstr[BUFFER];
        addrtostr(caddr, caddrstr, BUFFER);
        printf("[log] connection from %s\n", caddrstr);

        struct action msg;
        memset(&msg, 0, sizeof(msg));
        size_t count = recv(csock, &msg, sizeof(msg), 0);
        if (count > 0) {
            handle_message(csock, &msg, input_file);
        }

        close(csock);
    }

    close(s);
    return 0;
}