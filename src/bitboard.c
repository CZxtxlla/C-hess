#include "../include/bitboard.h"
#include <stdio.h>

void print_bitboard(U64 bitboard) {
    for (int rank = 7; rank >= 0; rank--) {
        printf(" %d  ", rank + 1); // print rank number

        for (int file = 0; file < 8; file++) {
            int square = rank * 8 + file;
            printf(" %llu ", get_bit(bitboard, square));
        }
        printf("\n");
    }

    // Print file labels
    printf("\n     a  b  c  d  e  f  g  h\n\n");
}