#include <stdio.h>

#include "../include/position.h"

int main() {
    Position board;
    
    // Parse the baseline standard tournament chess FEN string
    parse_fen(&board, START_POSITION);
    
    printf("--- Verifying FEN Parser State ---\n");
    print_board(&board);
    
    return 0;
}