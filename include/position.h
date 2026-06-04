#ifndef POSITION_H
#define POSITION_H

#include "types.h"

// castling rights (0001 for white kingside, 0010 for white queenside, ...)
enum {
    WK = 1, 
    WQ = 2,
    BK = 4,
    BQ = 8
};

typedef struct {

    U64 pieces[12]; // one bitboard for each piece type

    U64 occupancy[3]; // for fast checks of white pieces, black pieces, all pieces

    int side; // WHITE or BLACK
    int en_passant; // En passant square (-1 if there is none)
    int castling_rights; // 4 bit mask (WK | WQ | BK | BQ)


} Position;

#define START_POSITION "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1"

void parse_fen(Position* pos, const char* fen); // load fen into position struct
void print_board(const Position* pos); // print fen from position struct

#endif