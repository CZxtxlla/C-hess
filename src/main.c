#include <stdio.h>
#include <sys/time.h>
#include "../include/types.h"
#include "../include/bitboard.h"
#include "../include/magic.h"
#include "../include/position.h"
#include "../include/movegen.h"

// Forward declarations
long long perft(Position* pos, int depth);

void perft_divide(Position* pos, int depth);
void perft_divide_depth2(Position* pos, int depth);

// helper: find and make a move by from/to squares
static int find_and_make(Position* p, int from, int to) {
    MoveList tmp;
    generate_moves(p, &tmp);
    for (int i = 0; i < tmp.count; i++) {
        if (get_move_from(tmp.moves[i]) == from && get_move_to(tmp.moves[i]) == to) {
            return make_move(p, tmp.moves[i]);
        }
    }
    return 0;
}

void test_magic_vision() {
    printf("\n--- Queen Vision Test ---\n");
    
    // 1. Create a completely empty board
    U64 empty_board = 0ULL;
    
    // 2. Put a Queen on D4 (Index 27) and ask for her moves
    U64 q_attacks = get_queen_attacks(D4, empty_board);
    
    // 3. Print the resulting bitboard
    for (int rank = 7; rank >= 0; rank--) {
        for (int file = 0; file < 8; file++) {
            int square = rank * 8 + file;
            if (square == D4) printf(" Q ");
            else if (get_bit(q_attacks, square)) printf(" x ");
            else printf(" . ");
        }
        printf("\n");
    }
}

// Utility to get current time in milliseconds
long long get_time_ms() {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (tv.tv_sec * 1000LL) + (tv.tv_usec / 1000);
}

int main() {
    printf("--- Booting Engine ---\n");
    printf("Initializing Leaper Tables...\n");
    init_leapers();
    printf("Initializing Magic Bitboards...\n");
    init_sliders();
    printf("Initialization Complete.\n");

    Position board;
    parse_fen(&board, START_POSITION);
    print_board(&board);

    // Quick test: apply e2e4 e7e5 g1f3 g8f6 and list white moves (expect castling possible)
    Position test = board;
    MoveList tmpmoves;

    find_and_make(&test, E2, E4);
    find_and_make(&test, E7, E5);
    find_and_make(&test, G1, F3);
    find_and_make(&test, G8, F6);

    generate_moves(&test, &tmpmoves);
    printf("Moves after e2e4 e7e5 Nf3 Nf6 (white to move) - castling moves:\n");
    for (int i = 0; i < tmpmoves.count; i++) {
        if (get_move_castle(tmpmoves.moves[i])) {
            print_move(tmpmoves.moves[i]);
            printf("\n");
        }
    }

    int max_depth = 5; 
    
    printf("--- Starting Perft Test ---\n");
    for (int depth = 1; depth <= max_depth; depth++) {
        long long start_time = get_time_ms();
        
        // reset diagnostics
        extern long long diag_castles, diag_ep, diag_promotions;
        diag_castles = diag_ep = diag_promotions = 0;

        long long nodes = perft(&board, depth);
        
        long long end_time = get_time_ms();
        long long time_taken = end_time - start_time;
        
        // Prevent division by zero if it finishes in <1 ms
        long long nps = (time_taken > 0) ? (nodes * 1000) / time_taken : 0; 

        printf("Depth %d | Nodes: %-10lld | Time: %-6lld ms | NPS: %lld | castles=%lld ep=%lld promos=%lld\n", 
               depth, nodes, time_taken, nps, diag_castles, diag_ep, diag_promotions);
    }
    perft_divide(&board, 4);
    perft_divide_depth2(&board, 3);

    test_magic_vision();
    
    return 0;
}