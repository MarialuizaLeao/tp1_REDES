#include "common.h"

int board[MAX][MAX];

void initArgs(int argc, char *argv[]){
    if(argc != 3){
        errorHandler(CLIENT_USAGE_ERROR);
        exit(1);
    }
    else{
        ipVersion = argv[1];
        port = argv[2];
    }
}

bool validCell(struct action action){
    if(action.coordinates[0] < 0 || action.coordinates[0] >= MAX || action.coordinates[1] < 0 || action.coordinates[1] >= MAX){
        errorHandler(INVALID_CELL_ERROR);
        return false;
    }
    return true;
}

bool validAction(struct action action){
    if(action.type == REVEAL && REVEALED(action.board[action.coordinates[0]][action.coordinates[1]])){
        errorHandler(REVEAL_ALREADY_REVEALED_CELL_ERROR);
        return false;
    }
    else if(action.type == FLAG && action.board[action.coordinates[0]][action.coordinates[1]] == FLAGGED){
        errorHandler(FLAG_ALREADY_FLAGGED_CELL_ERROR);
        return false;
    }
    else if(action.type == FLAG && REVEALED(action.board[action.coordinates[0]][action.coordinates[1]])){
        errorHandler(FLAG_ALREADY_REVEALED_CELL_ERROR);
        return false;
    }
    return true;
}

// Checagem de qual comando foi chamado
int parseCommand(char *cmd){
    char *command = strtok(cmd, " ");
    if(strcmp(command, "start") == 0){
        return START;
    }
    else if(strcmp(command, "reveal") == 0){
        return REVEAL;
    }
    else if(strcmp(command, "flag") == 0){
        return FLAG;
    }
    else if(strcmp(command, "remove_flag") == 0){
        return REMOVE_FLAG;
    }
    else if(strcmp(command, "reset") == 0){
        return RESET;
    }
    else if(strcmp(command, "exit") == 0){
        return EXIT;
    }
    return ERROR;
}

int main(int argc, char **argv){
    initArgs(argc, argv);
    //Reset board

    struct sockaddr_storage storage;
    if(0 != addrparse(ipVersion, port, &storage)){
        logexit("addrparse");
    }

    // Inicializar Socket
    int s;
    s = socket(storage.ss_family, SOCK_STREAM, 0);
    if(s == -1){
        logexit("socket");
    }

    // Inicializar Conexao
    struct sockaddr *addr = (struct sockaddr *)(&storage); 
    if(0 != connect(s, addr, sizeof(storage))){
        logexit("connect");
    }

    char msg[BUFSZ];

    bool gameStarted = false;

    while(1){
        fflush(stdin);
        memset(msg, 0, BUFSZ);

        //Leitura das mensagens de comando
        scanf("%s", msg);

        // Quebra da mensagem para analisar elementos
        int cmdType = parseCommand(msg);
        
        int coordnates[2];

        bool canSend = false;
        struct action request;

        // Chamada de ação específica para cada tipo de comando enviado
        switch(cmdType){
            case ERROR:
                errorHandler(COMMAND_ERROR);
                break;
            case START:
                gameStarted = true;
                request = initAction(START, coordnates, board);
                canSend = true;
                break;
            case REVEAL:
                scanf("%d,%d", &coordnates[0], &coordnates[1]);
                request = initAction(REVEAL, coordnates, board);
                if(validCell(request) && validAction(request)){
                    canSend = true;
                }
                break;
            case FLAG:
                scanf("%d,%d", &coordnates[0], &coordnates[1]);
                request = initAction(FLAG, coordnates, board);
                if(validCell(request) && validAction(request)){
                    canSend = true;
                }
                break;
            case REMOVE_FLAG:
                scanf("%d,%d", &coordnates[0], &coordnates[1]);
                request = initAction(REMOVE_FLAG, coordnates, board);
                if(validCell(request)){
                    canSend = true;
                }
                break;
            case RESET:
                request = initAction(RESET, coordnates, board);
                canSend = true;
                break;
            case EXIT:
                printf("exiting...\n");
                close(s);
                break;
            default:
                break;
        }
        if(canSend){
            int count = send(s, &request, sizeof(request), 0);
            if(count != sizeof(request)){
                logexit("send");
            }
            struct action response;
            count = recv(s, &response, sizeof(response), 0);
            if (response.type == WIN) {
                printf("YOU WIN!\n");
                gameStarted = false;
            } else if(response.type == GAME_OVER){
                printf("GAME OVER!\n");
                gameStarted = false;
            }

            printBoard(response.board);  
        }
        exit(EXIT_SUCCESS);
    }
}