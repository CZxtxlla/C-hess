#include <stdio.h>
#include "../include/position.h"
#include "../include/movegen.h"

// Recursive function to count nodes
long long perft(Position* pos, int depth) {
    if (depth == 0) return 1ULL;

    MoveList list;
    generate_moves(pos, &list);
    
    long long nodes = 0;

    for (int i = 0; i < list.count; i++) {
        Position next_state = *pos;
        
        // Only count the move if it is legally valid
        if (make_move(&next_state, list.moves[i])) {
            // diagnostics: increment counters for special move types
            extern long long diag_castles, diag_ep, diag_promotions;
            if (get_move_castle(list.moves[i])) diag_castles++;
            if (get_move_ep(list.moves[i])) diag_ep++;
            if (get_move_promoted(list.moves[i])) diag_promotions++;

            nodes += perft(&next_state, depth - 1);
        }
    }
    
    return nodes;
}

void perft_divide(Position* pos, int depth) {
    MoveList list;
    generate_moves(pos, &list);
    long long total_nodes = 0;

    printf("--- Perft Divide (Depth %d) ---\n", depth);
    for (int i = 0; i < list.count; i++) {
        Position next_state = *pos;
        
        // Count generated moves at depth 1 for this branch
        MoveList child_list;
        Position child_state = next_state;
        if (make_move(&child_state, list.moves[i])) {
            generate_moves(&child_state, &child_list);
        } else {
            child_list.count = 0;
        }

        int gen_count = child_list.count;
        int legal_count = 0;
        // count how many child moves are legal (make_move succeeds)
        for (int j = 0; j < child_list.count; j++) {
            Position t = child_state;
            if (make_move(&t, child_list.moves[j])) legal_count++;
        }

        if (make_move(&next_state, list.moves[i])) {
            long long nodes = perft(&next_state, depth - 1);
            
            // Print the move, generated/legal child counts, and its branch node count
            print_move(list.moves[i]);
            printf(": gen=%d legal=%d nodes=%lld\n", gen_count, legal_count, nodes);
            
            total_nodes += nodes;
        } else {
            // move illegal at root (shouldn't happen normally)
            print_move(list.moves[i]);
            printf(": ILLEGAL at root\n");
        }
    }
    printf("Total Nodes: %lld\n", total_nodes);
}

// Diagnostic: for each root move, list each child move and the perft nodes for depth-2
void perft_divide_depth2(Position* pos, int depth) {
    MoveList list;
    generate_moves(pos, &list);

    printf("--- Perft Divide Depth2 (root depth %d) ---\n", depth);
    for (int i = 0; i < list.count; i++) {
        Position next_state = *pos;
        if (!make_move(&next_state, list.moves[i])) continue;

        // generate child moves
        MoveList child_list;
        generate_moves(&next_state, &child_list);

        print_move(list.moves[i]);
        printf(": total_children=%d\n", child_list.count);

        for (int j = 0; j < child_list.count; j++) {
            Position g = next_state;
            if (!make_move(&g, child_list.moves[j])) continue;
            long long nodes = perft(&g, depth - 2);
            print_move(child_list.moves[j]);
            printf(" -> %lld\n", nodes);
        }
        printf("\n");
    }
}

// Diagnostic counters (set externally before running perft)
long long diag_castles = 0;
long long diag_ep = 0;
long long diag_promotions = 0;