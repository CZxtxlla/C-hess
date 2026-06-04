#include "../include/bitboard.h"
#include <stdio.h>

U64 knight_attacks[64];
U64 king_attacks[64];

U64 not_a_file = 0xFEFEFEFEFEFEFEFEULL;
U64 not_h_file = 0x7F7F7F7F7F7F7F7FULL;
U64 not_ab_file = 0xFCFCFCFCFCFCFCFCULL;
U64 not_gh_file = 0x3F3F3F3F3F3F3F3FULL;

void init_leapers() {
    // initialize king and knight move arrays
    for (int square = 0; square < 64; square++) {
        U64 bitboard = 0ULL;
        set_bit(bitboard, square);

        U64 knight = 0ULL;
        
        // up-left and up-right
        knight |= (bitboard << 15) & not_h_file;
        knight |= (bitboard << 17) & not_a_file;

        // down-left and down-right
        knight |= (bitboard >> 15) & not_a_file;
        knight |= (bitboard >> 17) & not_h_file;

        // right-up and right-down
        knight |= (bitboard << 10) & not_ab_file;
        knight |= (bitboard >> 6) & not_ab_file;

        // left-up and left-down
        knight |= (bitboard << 6) & not_gh_file;
        knight |= (bitboard >> 10) & not_gh_file;

        knight_attacks[square] = knight;

        U64 king = 0ULL;

        // up down left right
        king |= (bitboard << 8);
        king |= (bitboard >> 8);
        king |= (bitboard << 1) & not_a_file;
        king |= (bitboard >> 1) & not_h_file;

        // diagonals
        king |= (bitboard << 9) & not_a_file;
        king |= (bitboard >> 7) & not_a_file;
        king |= (bitboard << 7) & not_h_file;
        king |= (bitboard >> 9) & not_h_file;

        king_attacks[square] = king;
    }
}

U64 mask_bishop_attacks(int square) {
    U64 attacks = 0ULL;
    int r, f;
    int file = square % 8;
    int rank = square / 8;

    for (r = rank + 1, f = file + 1; r <= 6 && f <= 6; r++, f++) {
        set_bit(attacks, r * 8 + f);
    }
    for (r = rank + 1, f = file - 1; r <= 6 && f >= 1; r++, f--) {
        set_bit(attacks, r * 8 + f);
    }
    for (r = rank - 1, f = file + 1; r >= 1 && f <= 6; r--, f++) {
        set_bit(attacks, r * 8 + f);
    }
    for (r = rank - 1, f = file - 1; r >= 1 && f >= 1; r--, f--) {
        set_bit(attacks, r * 8 + f);
    }

    return attacks;
}

U64 mask_rook_attacks(int square) {
    U64 attacks = 0ULL;

    int file = square % 8;
    int rank = square / 8;

    for (int r = rank + 1; r <= 6; r++) {
        set_bit(attacks, r * 8 + file);
    }
    for (int r = rank - 1; r >= 1; r--) {
        set_bit(attacks, r * 8 + file);
    }
    for (int f = file + 1; f <= 6; f++) {
        set_bit(attacks, rank * 8 + f);
    }
    for (int f = file - 1; f >= 1; f--) {
        set_bit(attacks, rank * 8 + f);
    }

    return attacks;
}

U64 set_occupancy(int index, int bits_in_mask, U64 attack_mask) {
    // maps index number [0 to 2^(bits_in_mask)] to bitboard subset of mask bitboard

    U64 occupancy = 0ULL;

    for (int count = 0; count < bits_in_mask; count++) {
        // loop through each bit in the mask and add it to the subset if it is specified in the index
        int square = __builtin_ctzll(attack_mask);

        pop_bit(attack_mask, square);

        if (index & (1 << count)) {
            // put a piece on the square if it's bit is 1
            set_bit(occupancy, square);
        }
    }

    return occupancy;
}

U64 bishop_attacks_cast(int square, U64 block) {
    U64 attacks = 0ULL;
    int r, f;
    int file = square % 8;
    int rank = square / 8;

    for (r = rank + 1, f = file + 1; r <= 7 && f <= 7; r++, f++) {
        set_bit(attacks, r * 8 + f);
        if (get_bit(block, r * 8 + f)) {
            break;
        }
    }
    for (r = rank + 1, f = file - 1; r <= 7 && f >= 0; r++, f--) {
        set_bit(attacks, r * 8 + f);
        if (get_bit(block, r * 8 + f)) {
            break;
        }
    }
    for (r = rank - 1, f = file + 1; r >= 0 && f <= 7; r--, f++) {
        set_bit(attacks, r * 8 + f);
        if (get_bit(block, r * 8 + f)) {
            break;
        }
    }
    for (r = rank - 1, f = file - 1; r >= 0 && f >= 0; r--, f--) {
        set_bit(attacks, r * 8 + f);
        if (get_bit(block, r * 8 + f)) {
            break;
        }
    }

    return attacks;
}

U64 rook_attacks_cast(int square, U64 block) {
    U64 attacks = 0ULL;
    int r, f;
    int rank = square / 8;
    int file = square % 8;

    for (r = rank + 1; r <= 7; r++) {
        set_bit(attacks, r * 8 + file);
        if (get_bit(block, r * 8 + file)) {
            break;
        }
    }
    for (r = rank - 1; r >= 0; r--) {
        set_bit(attacks, r * 8 + file);
        if (get_bit(block, r * 8 + file)) {
            break;
        }
    }
    for (f = file + 1; f <= 7; f++) {
        set_bit(attacks, rank * 8 + f);
        if (get_bit(block, rank * 8 + f)) {
            break;
        }
    }
    for (f = file - 1; f >= 0; f--) {
        set_bit(attacks, rank * 8 + f);
        if (get_bit(block, rank * 8 + f)) {
            break;
        }
    }

    return attacks;
}


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