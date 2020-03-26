#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#include <assert.h>

#define BOARD_TYPE char
#define DEBUG 0
#define K_MAX 20
#define PLAYER1 0
#define PLAYER2 0
#define NUM_GAMES 100

struct ttt{
    BOARD_TYPE state;
    BOARD_TYPE board[9];
};

struct uttt{
    BOARD_TYPE state;
    struct ttt t[9];
};

unsigned rng(){
    unsigned r;
    asm ( "rdrand %0"
            : "=r" (r));
    //printf("%u\n", r);
    return r;
}

BOARD_TYPE t_check_win(struct ttt *t){
    //check each row
    for(int row = 0; row < 3; row++){
        BOARD_TYPE piece = t->board[row * 3];
        for(int col = 1; col < 3; col++){
            if(t->board[(row * 3) + col] != piece) piece = 0;
        }
        if(piece) return piece;
    }

    //check each column
    for(int col = 0; col < 3; col++){
        BOARD_TYPE piece = t->board[col];
        for(int row = 1; row < 3; row++){
            if(t->board[col + (row * 3)] != piece) piece = 0;
        }
        if(piece) return piece;
    }

    //check each diagonal
    BOARD_TYPE piece = t->board[0];
    if(!(piece == t->board[4] && piece == t->board[8])) piece = 0;
    if(piece) return piece;

    piece = t->board[2];
    if(!(piece == t->board[4] && piece == t->board[6])) piece = 0;
    if(piece) return piece;

    //check for a draw
    for(int i = 0; i < 9; i++){
        if(t->board[i] == 0) return 0;
    }

    return 'D';
}

int place_tile(struct ttt *t, int location, BOARD_TYPE player){
    if(t->state) return 0;
    if(t->board[location] != 0) return 0;
    t->board[location] = player;
    t->state = t_check_win(t);
    //printf("placed_tile %c in location %d\n", player, location);
    return 1;
}

BOARD_TYPE ut_check_win(struct uttt *ut){
    //check each row
    for(int row = 0; row < 3; row++){
        BOARD_TYPE piece = ut->t[row * 3].state;
        if(piece == 'D') break;
        for(int col = 1; col < 3; col++){
            if(ut->t[(row * 3) + col].state != piece) piece = 0;
        }
        if(piece){
            //printf("ROW VICTORY\n");
            return piece;
        }
    }

    //check each column
    for(int col = 0; col < 3; col++){
        BOARD_TYPE piece = ut->t[col].state;
        if(piece == 'D') break;
        for(int row = 1; row < 3; row++){
            if(ut->t[col + (row * 3)].state != piece) piece = 0;
        }
        if(piece){
            //printf("COLUMN VICTORY\n");
            return piece;
        } 
    }

    //check each diagonal
    BOARD_TYPE piece = ut->t[0].state;
    if(piece == 'D') piece = 0;
    if(!(piece == ut->t[4].state && piece == ut->t[8].state)) piece = 0;
    if(piece) return piece;

    piece = ut->t[2].state;
    if(piece == 'D') piece = 0;
    if(!(piece == ut->t[4].state && piece == ut->t[6].state)) piece = 0;
    if(piece) return piece;

    //check for a draw
    for(int i = 0; i < 9; i++){
        if(ut->t[i].state == 0) return 0;
    }

    return 'D';
}

void print_ttt(struct ttt *t){
    for(int i = 0; i < 9; i++){
        if((i % 3) == 0) printf("\n");
        if(t->board[i] == 0){ 
            printf("#");
            continue;
        }
        printf("%c", t->board[i]);
    }
    printf("\n");
}

void print_uttt(struct uttt *ut){
    printf("=============\n\n");
    //BOARD_TYPE board[9][9];
    for(int row = 0; row < 9; row++){
        for(int col = 0; col < 9; col++){
            if(!(col % 3)) printf(" ");
            int x = col / 3;
            int y = row / 3;
            BOARD_TYPE c = ut->t[(y * 3) + x].board[((row % 3) * 3) + (col % 3)];
            if(c == 0){
                printf("#");
                continue;
            }
            printf("%c", c);
        }
        if(!((row+1) % 3)) printf("\n");
        printf("\n");
    }
    printf("=============\n");
}

void init_uttt(struct uttt *ut){
    memset(ut, 0, sizeof(struct uttt));
}

int random_agent(struct uttt *ut, unsigned to_play, BOARD_TYPE player, int k_max){
    //if board to play is full, reset it 
    while(ut->t[to_play].state) to_play = rng() % 9;
    while(1){
        //printf("playing in board %u\n", to_play);
        volatile unsigned rand = rng() % 9;
        //printf("to play %d\n", to_play);
        int placed = place_tile(&ut->t[to_play], rand, player);
        if(placed){ 
            //check game over in sub-game  
            //ut->t[to_play].state = t_check_win(&ut->t[to_play]);
            return rand;
        }
    }
}


//random decisions for now
BOARD_TYPE utt_continue(struct uttt *ut, unsigned to_play, BOARD_TYPE player, int *subgame_x_count, int *subgame_o_count, int (*player1)(struct uttt *, unsigned, BOARD_TYPE, int), int (*player2)(struct uttt *, unsigned, BOARD_TYPE, int), int k_max){
    //BOARD_TYPE players[2] = {'X', 'O'};
    //unsigned to_play = rng() % 9;
    //game loop
    while(1){
        BOARD_TYPE ret = ut_check_win(ut);
        if(ret){
            //int i = 4;
            for(int i = 0; i < 9; i++){
                if(ut->t[i].state == 'X') *subgame_x_count += 1;
                if(ut->t[i].state == 'O') *subgame_o_count += 1;
            }
            //char buf;
            //scanf("%c", &buf);
            //print_uttt(ut);
            return ret;
        }
        //selection loop
        to_play = (player == 'X') ? player1(ut, to_play, player, k_max) : player2(ut, to_play, player, k_max);
        //print_uttt(&ut); 
        //check victory

        //increment player
        player = (player == 'X') ? 'O' : 'X';
        //player++;
        //player %= 2;

    }

    //print_uttt(ut);
}

int mcts_agent(struct uttt *ut, unsigned to_play, BOARD_TYPE player, int k_max){
    //int k_max = 50;
    //while(ut->t[to_play].state) to_play = rng() % 9;
    int max_games = 0;
    int saved_game = -1;
    int saved_position = -1;
    int iterations = (ut->t[to_play].state) ? 9 : 1; 
    //printf("player %c\n", player);
    for(int i = 0; i < iterations; i++){
        for(int position = 0; position < 9; position++){
            struct uttt test_ut = *ut;
            //printf("%p\n%p\n", ut, &test_ut);
            int placed = place_tile(&test_ut.t[(to_play + i) % 9], position, player);
            struct uttt test_ut_backup = test_ut;
            if(placed){
                int games_won = 0;
                for(int k = 0; k < k_max; k++){
                    test_ut = test_ut_backup;
                    char next_player = (player == 'X') ? 'O' : 'X';
                    int subgame_x_count, subgame_o_count = 0;
                    BOARD_TYPE result = utt_continue(&test_ut, position, next_player, &subgame_x_count, &subgame_o_count, random_agent, random_agent, k_max);
                    //printf("result: %c\n", result);
                    if(result == player) games_won++;
                }
                if(games_won > max_games){
                    max_games = games_won;
                    saved_game = (to_play + i) % 9;
                    saved_position = position;
                }
            }
        }
    }
    //printf("max_games: %d\n", max_games);
    if(saved_game == -1) return random_agent(ut, to_play, player, 0);
    int placed = place_tile(&ut->t[saved_game], saved_position, player);
    return saved_position;
}

int mcts_heuristic_agent(struct uttt *ut, unsigned to_play, BOARD_TYPE player, int k_max){
    //int k_max = 10;
    //while(ut->t[to_play].state) to_play = rng() % 9;
    int max_games = 0;
    int saved_game = -1;
    int saved_position = -1;
    int iterations = (ut->t[to_play].state) ? 9 : 1; 
    //printf("player %c\n", player);
    for(int i = 0; i < iterations; i++){
        for(int position = 0; position < 9; position++){
            struct uttt test_ut = *ut;
            //printf("%p\n%p\n", ut, &test_ut);
            int placed = place_tile(&test_ut.t[(to_play + i) % 9], position, player);
            struct uttt test_ut_backup = test_ut;
            if(placed){
                int games_won = 0;
                for(int k = 0; k < k_max; k++){
                    test_ut = test_ut_backup;
                    char next_player = (player == 'X') ? 'O' : 'X';
                    int subgame_x_count, subgame_o_count = 0;
                    BOARD_TYPE result = utt_continue(&test_ut, position, next_player, &subgame_x_count, &subgame_o_count, random_agent, random_agent, 0);
                    //printf("result: %c\n", result);
                    if(result == player) games_won += 20;
                    games_won += (player == 'X') ? subgame_x_count : subgame_o_count;
                }
                if(games_won > max_games){
                    max_games = games_won;
                    saved_game = (to_play + i) % 9;
                    saved_position = position;
                }
            }
        }
    }
    //printf("max_games: %d\n", max_games);
    if(saved_game == -1) return random_agent(ut, to_play, player, 0);
    int placed = place_tile(&ut->t[saved_game], saved_position, player);
    return saved_position;
}

int main(){
    char *agent_names[3] = {"mcts", "mcts_h", "random"};
    int (*agents[3])(struct uttt *, unsigned, BOARD_TYPE, int) = {mcts_agent, mcts_heuristic_agent, random_agent};
    int d, x, o;
    int num_games = NUM_GAMES;
    printf("kmax,x_agent,o_agent,num_games,x_wins,o_wins,draws\n");
    for(int k = 0; k < 60; k += 10){
            int player1 = 0;
            for(int player2 = 0; player2 < 2; player2++){
            //int player2 = 0;
            d = x = o = 0;
            for(int i = 0; i < num_games; i++){
                struct uttt ut;
                init_uttt(&ut);
                int xc, oc = 0;
                char u = utt_continue(&ut, rng() % 9, 'X', &xc, &oc, agents[player1], agents[player2],k);
                switch(u){
                    case 'D':
                        d++;
                        break;
                    case 'X':
                        x++;
                        break;
                    case 'O':
                        o++;
                        break;
                }
                if(DEBUG) print_uttt(&ut);
            }
            printf("%d,%s,%s,%d,%d,%d,%d\n", k, agent_names[player1], agent_names[player2], num_games, x, o, d);

        }
    }
    return 0;
}
