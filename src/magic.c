#include "../include/magic.h"
#include "../include/bitboard.h"

// pre computed magic numbers (computed by someone else)
const U64 rook_magics[64] = {
    0x8a80104000800020ULL, 0x140002000100040ULL, 0x2801880a0017001ULL, 0x100081001000420ULL,
    0x200020010080420ULL, 0x3001c0002010008ULL, 0x8480008002000100ULL, 0x2080088004402900ULL,
    0x800098204000ULL, 0x2024401000200040ULL, 0x100802000801000ULL, 0x120800800801000ULL,
    0x208808088000400ULL, 0x2802200800400ULL, 0x2200800100020080ULL, 0x801000060821100ULL,
    0x80044006422000ULL, 0x100808020004000ULL, 0x12108a0010204200ULL, 0x140848010000802ULL,
    0x481828014002800ULL, 0x8094004002004100ULL, 0x4010040010010802ULL, 0x200040005040080ULL,
    0x800100122000000ULL, 0x100800140020000ULL, 0x800802002200800ULL, 0x800080200500140ULL,
    0x18801111400400ULL, 0x10010400414000ULL, 0x2008101008120ULL, 0x20484080120180ULL,
    0x444004414101100ULL, 0x800200102142200ULL, 0x804000200106800ULL, 0x10042420082040ULL,
    0x8114011244000ULL, 0x884000084010ULL, 0x18001180410084ULL, 0x1102002201081ULL,
    0x80040008004010ULL, 0x408024401018ULL, 0x2112004084040ULL, 0x4104008126300ULL,
    0x10040100042010ULL, 0x80424000820ULL, 0x242404050201ULL, 0x400212001088ULL,
    0x1208022020202ULL, 0x80004000808ULL, 0x140011400800ULL, 0x820104000800100ULL,
    0x81010140120400ULL, 0x2000102422040ULL, 0x8400420210ULL, 0x100802100810ULL,
    0x8000820014010ULL, 0x20000201040400ULL, 0x1000810102100ULL, 0x110081008000ULL,
    0x102021141ULL, 0x400208110000ULL, 0x11422041010ULL, 0x280422110200ULL
};

const U64 bishop_magics[64] = {
    0x40040844404084ULL, 0x2004208a004208ULL, 0x10190041080202ULL, 0x108060845042010ULL,
    0x581104180800210ULL, 0x211208044620004ULL, 0x41105040830020ULL, 0x108062022081210ULL,
    0x40080420080080ULL, 0x2000200200804ULL, 0x800400804004ULL, 0x2000000100400ULL,
    0x20020020080ULL, 0x41000214040ULL, 0x10041004201ULL, 0x441020020020ULL,
    0x40200200800ULL, 0x40202002004ULL, 0x100008002008ULL, 0x204000200020ULL,
    0x200040010001ULL, 0x201000100020ULL, 0x204020400ULL, 0x8001000200ULL,
    0x40008402004ULL, 0x4002008000ULL, 0x8100010001ULL, 0x800100020ULL,
    0x40000100200ULL, 0x8000002001ULL, 0x20010202ULL, 0x8000000400ULL,
    0x20002011ULL, 0x10000002ULL, 0x2000200100ULL, 0x400001020ULL,
    0x80020040ULL, 0x100040010ULL, 0x102002010ULL, 0x80002000ULL,
    0x40004ULL, 0x800400ULL, 0x40008000ULL, 0x100002ULL,
    0x8000100ULL, 0x20008ULL, 0x210001ULL, 0x800400020ULL,
    0x40020ULL, 0x2002010ULL, 0x200020010ULL, 0x8004000ULL,
    0x40002004ULL, 0x10000008ULL, 0x20008020ULL, 0x1001ULL,
    0x80002ULL, 0x1002ULL, 0x2100ULL, 0x4008ULL,
    0x20001ULL, 0x8ULL, 0x40002001000ULL, 0x4000ULL
};

// lookup tables
U64 rook_attacks[64][4096];
U64 bishop_attacks[64][512];
U64 rook_masks[64];
U64 bishop_masks[64];

// runs once when the engine boots up to build the tables
void init_sliders() {
    for (int square = 0; square < 64; square++) {
        rook_masks[square] = mask_rook_attacks(square);
        int r_bits = __builtin_popcountll(rook_masks[square]);
        int r_perm = (1 << r_bits); // 2^r_bits;

        for (int i = 0; i < r_perm; i++) {
            U64 occ = set_occupancy(i, r_bits, rook_masks[square]);

            // The Magic Hash Formula
            int magic_index = (occ * rook_magics[square]) >> (64 - r_bits);

            rook_attacks[square][magic_index] = rook_attacks_cast(square, occ);
        }

        bishop_masks[square] = mask_bishop_attacks(square);
        int b_bits = __builtin_popcountll(bishop_masks[square]);
        int b_perm = (1 << r_bits); // 2^r_bits;

        for (int i = 0; i < b_perm; i++) {
            U64 occ = set_occupancy(i, b_bits, bishop_masks[square]);

            // The Magic Hash Formula
            int magic_index = (occ * bishop_magics[square]) >> (64 - b_bits);

            bishop_attacks[square][magic_index] = bishop_attacks_cast(square, occ);
        }
    }
}


// super fast O(1) table lookups
U64 get_rook_attacks(int square, U64 occupancy) {
    U64 relevant_occupancy = occupancy & rook_masks[square];

    int r_bits = __builtin_popcountll(rook_masks[square]);
    int magic_index = (relevant_occupancy * rook_magics[square]) >> (64 - r_bits);

    return rook_attacks[square][magic_index];
}

U64 get_bishop_attacks(int square, U64 occupancy) {
    U64 relevant_occupancy = occupancy & bishop_masks[square];

    int b_bits = __builtin_popcountll(bishop_masks[square]);
    int magic_index = (relevant_occupancy * bishop_magics[square]) >> (64 - b_bits);

    return bishop_attacks[square][magic_index];
}

U64 get_queen_attacks(int square, U64 occupancy) {
    return get_rook_attacks(square, occupancy) | get_bishop_attacks(square, occupancy);
}