#ifndef BITBOARD_H
#define BITBOARD_H

#include "types.h"

// bitboard manipulation operations
#define get_bit(bitboard, square) (((bitboard) >> (square)) & 1ULL)
#define set_bit(bitboard, square) ((bitboard) |= (1ULL << (square)))
#define pop_bit(bitboard, square) ((bitboard) &= ~(1ULL << (square)))

// print a visual representation of the bitboard to stdout
void print_bitboard(U64 bitboard);


// pre computed since they don't depend on the board position
extern U64 knight_moves[64];
extern U64 king_moves[64];

void init_leapers(); // initialize the knight_moves and king_moves arrays


// ----- magic bitboards stuff ------

// masks for relevant occupancy squares
U64 mask_bishop_attacks(int square);
U64 mask_rook_attacks(int square);

// get the permutation corresponding to the index
U64 set_occupancy(int index, int bits_in_mask, U64 attack_mask);

// get legal moves for a certain occupancy board
U64 bishop_attacks_cast(int square, U64 block);
U64 rook_attacks_cast(int square, U64 block);

#endif