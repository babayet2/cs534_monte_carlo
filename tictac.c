#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>

#define BOARD_TYPE char

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
    return r;
}

int place_tile(struct ttt *t, int location, BOARD_TYPE player){
    if(t->state) return 0;
    if(t->board[location] != 0) return 0;
    t->board[location] = player;
    //printf("placed_tile %c in location %d\n", player, location);
    return 1;
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

int random_agent(struct uttt *ut, unsigned to_play, BOARD_TYPE player){
    //if board to play is full, reset it 
    while(ut->t[to_play].state) to_play = rng() % 9;
    while(1){
        //printf("playing in board %u\n", to_play);
        unsigned rand = rng() % 9;
        //printf("to play %d\n", to_play);
        int placed = place_tile(&ut->t[to_play], rand, player);
        if(placed){ 
            //check game over in sub-game  
            ut->t[to_play].state = t_check_win(&ut->t[to_play]);
            return rand;
        }
    }
}


//random decisions for now
BOARD_TYPE utt_continue(struct uttt *ut, unsigned to_play, int (*agent)(struct uttt *, unsigned, BOARD_TYPE)){
    int player = 0;
    BOARD_TYPE players[2] = {'X', 'O'};
    //unsigned to_play = rng() % 9;
    //game loop
    while(1){
        //selection loop
        to_play = agent(ut, to_play, players[player]);
        //print_uttt(&ut); 
        //check victory
        BOARD_TYPE ret = ut_check_win(ut);
        if(ret) return ret;

        //increment player
        player++;
        player %= 2;

        //char buf;
        //scanf("%c", &buf);
        //print_uttt(ut);
    }

    print_uttt(ut);
}

int mcts_agent(struct uttt *ut, unsigned to_play, BOARD_TYPE player){
    int k_max = 1;
    //int free = (int) ut->t[to_play].state;
    while(ut->t[to_play].state) to_play = rng() % 9;
    int max_games = 0;
    int saved_game, saved_position;
   // if(!free){
        for(int position = 0; position < 9; position++){
            struct uttt test_ut = *ut;
            int placed = place_tile(&test_ut.t[to_play], position, player);
            if(placed){
                int games_won = 0;
                for(int k = 0; k < k_max; k++){
                    BOARD_TYPE result = utt_continue(&test_ut, position, random_agent);
                    if(result == player) games_won++;
                }
                if(games_won > max_games){
                    max_games = games_won;
                    saved_game = to_play;
                    saved_position = position;
                }
            }
     //   }
    }/*
    else{
        for(int position = 0; position < 9; position++){
            for(to_play = 0; to_play < 9; to_play++){
                struct uttt test_ut = *ut;
                int placed = place_tile(&test_ut.t[to_play], position, player);
                if(placed){
                    int games_won = 0;
                    for(int k = 0; k < k_max; k++){
                        BOARD_TYPE result = utt_continue(&test_ut, position, random_agent);
                        if(result == player) games_won++;       
                    if(games_won > max_games){
                        max_games = games_won;
                        saved_game = to_play;
                        saved_position = position;
                    }
                }
            }
        
    }
*/
    int placed = place_tile(&ut->t[saved_game], saved_position, player);
    ut->t[to_play].state = t_check_win(&ut->t[to_play]);
    return saved_position;
}

int main(){
    int d, x, o;
    d = x = o = 0;
    for(int i = 0; i < 1; i++){
        struct uttt ut;
        init_uttt(&ut);
        char u = utt_continue(&ut, rng() % 9, mcts_agent);
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
    }
    printf("d %d | x %d | o %d\n", d, x, o);
    return 0;
}
