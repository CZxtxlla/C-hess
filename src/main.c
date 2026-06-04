#include <stdio.h>
#include "../include/position.h"
#include "../include/movegen.h"
#include "../include/bitboard.h"

int main() {

    init_leapers(); // initialize king and knight arrays;

    // 1. Get the relevant mask for a Rook on D4 (Square 27)
    U64 mask = mask_rook_attacks(A1);
    
    // 2. Count how many bits are in the mask using a compiler intrinsic
    int bit_count = __builtin_popcountll(mask); // Will be 10 for a central Rook
    
    // 3. Generate permutation #400 (out of 1024)
    U64 board_permutation = set_occupancy(400, bit_count, mask);
    
    print_bitboard(mask);              // Shows the empty cross missing the edges
    print_bitboard(board_permutation); // Shows a random scattering of pieces exactly on that cross

    return 0;
}