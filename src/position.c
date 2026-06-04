#include "../include/position.h"
#include "../include/types.h"
#include "../include/bitboard.h"
#include <stdio.h>
#include <string.h>

int char_to_piece(char c) {
    // takes character and converts it to corresponding int value
    switch (c) {
        case 'P': return P; case 'N': return N; case 'B': return B; 
        case 'R': return R; case 'Q': return Q; case 'K': return K;
        case 'p': return p; case 'n': return n; case 'b': return b; 
        case 'r': return r; case 'q': return q; case 'k': return k;
        default: return -1;
    }
}

char piece_to_char(int piece) {
    // takes int value and converts it to correpsonding character
    char pieces[] = "PNBRQKpnbrqk";
    return pieces[piece];
}

void parse_fen(Position* pos, const char* fen) {
    memset(pos, 0, sizeof(Position)); // fully reset pos structure
    pos->en_passant = -1;

    const char* ptr = fen;

    // piece placement data
    for (int rank = 7; rank >= 0; rank--) {
        for (int file = 0; file < 8; file++) {
            int square = rank * 8 + file;

            // perform boundary check before calling char_to_piece
            if ((*ptr >= 'a' && *ptr <= 'z') || (*ptr >= 'A' && *ptr <= 'Z')) {
                int piece = char_to_piece(*ptr);
                if (piece != -1) {
                    set_bit(pos->pieces[piece], square);
                }
                ptr++;
            } else if (*ptr >= '1' && *ptr <= '8') {
                int empty_squares = *ptr - '0';
                file += (empty_squares - 1);
                ptr++;
            }
        }
        if (*ptr == '/') {
            ptr++;
        }
    }

    // empty space
    if (*ptr == ' ') {
        ptr++;
    }

    // side to move
    if (*ptr == 'w') {
        pos->side = WHITE;
    } else if (*ptr == 'b') {
        pos->side = BLACK;
    }
    ptr +=2;

    // castling rights
    while (*ptr != ' ') {
        if (*ptr == 'K') pos->castling_rights |= WK;
        if (*ptr == 'Q') pos->castling_rights |= WQ;
        if (*ptr == 'k') pos->castling_rights |= BK;
        if (*ptr == 'q') pos->castling_rights |= BQ;
        ptr++;
    }
    ptr++; // skip the space

    // en passant square
    if (*ptr != '-') {
        int file = ptr[0] - 'a';
        int rank = ptr[1] - '0';
        pos->en_passant = file * 8 + rank;
        ptr +=2;
    } else {
        ptr++;
    }

    // occupancy bitboards
    for (int piece = P; piece <= K; piece++) {
        pos->occupancy[WHITE] |= pos->pieces[piece];
    }

    for (int piece = p; piece <= k; piece++) {
        pos->occupancy[BLACK] |= pos->pieces[piece];
    }
    pos->occupancy[BOTH] = pos->occupancy[BLACK] | pos->occupancy[WHITE];
}

void print_board(const Position* pos) {
    // print out board layout with ascii
    for (int rank = 7; rank >= 0; rank--) {
        printf(" %d ", rank + 1);
        for (int file = 0; file < 8; file++) {
            int square = rank * 8 + file;
            int piece = -1;

            // Determine if a piece occupies this specific square
            for (int p_type = P; p_type <= k; p_type++) {
                if (get_bit(pos->pieces[p_type], square)) {
                    piece = p_type;
                    break;
                }
            }
            printf(" %c", (piece == -1) ? '.' : piece_to_char(piece));
        }
        printf("\n");
    }
    printf("\n    a b c d e f g h\n\n");
    printf("  Side to move:  %s\n", (pos->side == WHITE) ? "white" : "black");
    printf("  Castling:      %c%c%c%c\n", 
           (pos->castling_rights & WK) ? 'K' : '-', (pos->castling_rights & WQ) ? 'Q' : '-',
           (pos->castling_rights & BK) ? 'k' : '-', (pos->castling_rights & BQ) ? 'q' : '-');
    printf("  En Passant:    %s\n", (pos->en_passant == -1) ? "none" : "active");
}