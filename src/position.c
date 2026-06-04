#include "../include/position.h"
#include "../include/types.h"
#include "../include/bitboard.h"
#include "../include/magic.h"
#include "../include/movegen.h"
#include <stdio.h>
#include <string.h>

// Castling rights update array (64 squares)
const int castling_rights_update[64] = {
    13, 15, 15, 15, 12, 15, 15, 14,  // Rank 1: A1 (13), E1 (12), H1 (14)
    15, 15, 15, 15, 15, 15, 15, 15,
    15, 15, 15, 15, 15, 15, 15, 15,
    15, 15, 15, 15, 15, 15, 15, 15,
    15, 15, 15, 15, 15, 15, 15, 15,
    15, 15, 15, 15, 15, 15, 15, 15,
    15, 15, 15, 15, 15, 15, 15, 15,
     7, 15, 15, 15,  3, 15, 15, 11   // Rank 8: A8 (7), E8 (3), H8 (11)
};

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

    // en passant square (file then rank in FEN, ranks are 1-based)
    if (*ptr != '-') {
        int file = ptr[0] - 'a';
        int rank = ptr[1] - '1';
        pos->en_passant = rank * 8 + file;
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

int is_square_attacked(int square, int attacker_side, const Position* pos) {
    U64 a_file = 0x0101010101010101ULL;
    U64 h_file = 0x8080808080808080ULL;

    // attacked by pawns? (pretend pawn on square and see if it attacks enemy pawn)
    if (attacker_side == WHITE) {
        if (((1ULL << square) >> 7) & ~a_file & pos->pieces[P]) {
            return 1;
        }
        if (((1ULL << square) >> 9) & ~h_file & pos->pieces[P]) {
            return 1;
        }
    } else {
        if (((1ULL << square) << 7) & ~h_file & pos->pieces[p]) {
            return 1;
        }
        if (((1ULL << square) << 9) & ~a_file & pos->pieces[p]) {
            return 1;
        }
    }

    // attacked by knights? (pretend knight is on square and see if it hits the enemy knights)
    U64 enemy_knights = (attacker_side == WHITE) ? pos->pieces[N] : pos->pieces[n];
    if (knight_attacks[square] & enemy_knights) return 1;
    
    // attacked by kings? (pretend king is on square and see if it hits the enemy king)
    U64 enemy_king = (attacker_side == WHITE) ? pos->pieces[K] : pos->pieces[k];
    if (king_attacks[square] & enemy_king) return 1;
    
    // attacked by bishops or queens? (pretend bishop on square and see if it hits an enemy bishop)
    U64 enemy_bishops_queens = (attacker_side == WHITE) ? (pos->pieces[B] | pos->pieces[Q]) : (pos->pieces[b] | pos->pieces[q]);
    if (get_bishop_attacks(square, pos->occupancy[BOTH]) & enemy_bishops_queens) {
        return 1;
    }

    // attacked by rooks or queens? (pretend rook on square and see if it hits an enemy rook)
    U64 enemy_rooks_queens = (attacker_side == WHITE) ? (pos->pieces[R] | pos->pieces[Q]) : (pos->pieces[r] | pos->pieces[q]);
    if (get_rook_attacks(square, pos->occupancy[BOTH]) & enemy_rooks_queens) {
        return 1;
    }

    return 0; // not attacked
}


int make_move(Position* pos, int move) {
    Position backup = *pos; // backup in case move is illegal and need to revert

    int from_sq = get_move_from(move);
    int to_sq = get_move_to(move);
    int promoted = get_move_promoted(move);
    int is_ep = get_move_ep(move);
    int is_double = get_move_double(move);
    int is_castling = get_move_castle(move);

    // find the moving piece
    int moving_piece = -1;
    for (int p_type = P; p_type <= k; p_type++) {
        if (get_bit(pos->pieces[p_type], from_sq)) {
            moving_piece = p_type;
            break;
        }
    }

    // If we land on a square, completely clear it from all piece bitboards
    for (int p_type = P; p_type <= k; p_type++) {
        pop_bit(pos->pieces[p_type], to_sq);
    }
    // move the piece
    pop_bit(pos->pieces[moving_piece], from_sq);
    set_bit(pos->pieces[moving_piece], to_sq);

    // special cases
    if (is_ep) {
        // handle en passant
        int captured_pawn_sq = (pos->side == WHITE) ? (to_sq - 8) : (to_sq + 8);
        int captured_pawn = (pos->side == WHITE) ? p : P;
        pop_bit(pos->pieces[captured_pawn], captured_pawn_sq); 

    } else if (promoted) {
        // handle promotion: remove pawn of moving side, add promoted piece
        if (pos->side == WHITE) {
            pop_bit(pos->pieces[P], to_sq);
        } else {
            pop_bit(pos->pieces[p], to_sq);
        }
        set_bit(pos->pieces[promoted], to_sq);

    } else if (is_castling) {
        // handle castle
        if (to_sq == G1) {
            // white kingside
            pop_bit(pos->pieces[R], H1);
            set_bit(pos->pieces[R], F1);
        } else if (to_sq == C1) {
            // white queenside
            pop_bit(pos->pieces[R], A1); 
            set_bit(pos->pieces[R], D1);
        } else if (to_sq == G8) {
            // black kingside
            pop_bit(pos->pieces[r], H8); 
            set_bit(pos->pieces[r], F8);
        } else if (to_sq == C8) {
            // black queenside
            pop_bit(pos->pieces[r], A8); 
            set_bit(pos->pieces[r], D8);
        }
    }

    if (is_double) {
        pos->en_passant = (pos->side == WHITE) ? (to_sq - 8) : (to_sq + 8);
    } else {
        pos->en_passant = -1;
    }  
    // update castling rights on squares touched
    pos->castling_rights &= castling_rights_update[from_sq];
    pos->castling_rights &= castling_rights_update[to_sq];

    // recalculate occupancy

    pos->occupancy[WHITE] = 0ULL;
    pos->occupancy[BLACK] = 0ULL;
    
    for (int p_type = P; p_type <= K; p_type++) {
        pos->occupancy[WHITE] |= pos->pieces[p_type];
    }
    for (int p_type = p; p_type <= k; p_type++) {
        pos->occupancy[BLACK] |= pos->pieces[p_type];
    }
    pos->occupancy[BOTH] = pos->occupancy[WHITE] | pos->occupancy[BLACK];

    pos->side ^= 1; // change turns

    // check if the move left the king in check (then it's illegal)

    int king_type = (pos->side == WHITE) ? k : K;
    int king_sq = __builtin_ctzll(pos->pieces[king_type]);

    if (is_square_attacked(king_sq, pos->side, pos)) {
        // illegal move
        *pos = backup;
        return 0;
    }

    // legal move
    return 1;
}