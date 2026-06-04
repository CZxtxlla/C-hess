#ifndef BITBOARD_H
#define BITBOARD_H

#include "types.h"

// bitboard manipulation operations
#define get_bit(bitboard, square) (((bitboard) >> (square)) & 1ULL)
#define set_bit(bitboard, square) ((bitboard) |= (1ULL << (square)))
#define pop_bit(bitboard, square) ((bitboard) &= ~(1ULL << (square)))

// print a visual representation of the bitboard to stdout
void print_bitboard(U64 bitboard);


#endif