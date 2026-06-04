#include "../include/movegen.h"
#include "../include/bitboard.h"

void addMove(MoveList* list, int move) {
    list->moves[list->count] = move;
    list->count++;
}

void generate_moves(const Position* pos, MoveList* list) {
    list->count = 0;
    U64 bitboard, pushes;
    int to_sq, from_sq;

    if (pos->side == WHITE) {
        bitboard = pos->pieces[P];

        pushes = (bitboard) << 8 & ~pos->occupancy[BOTH]; // set move

        // single pushes
        U64 single_pushes = pushes;
        while (single_pushes) {
            to_sq = __builtin_ctzll(single_pushes);

            from_sq = to_sq - 8;

            if (to_sq >= A8 & to_sq <= H8) {
                addMove(list, encode_move(from_sq, to_sq, Q, 0, 0, 0));
                addMove(list, encode_move(from_sq, to_sq, R, 0, 0, 0));
                addMove(list, encode_move(from_sq, to_sq, B, 0, 0, 0));
                addMove(list, encode_move(from_sq, to_sq, N, 0, 0, 0));
            } else {
                addMove(list, encode_move(from_sq, to_sq, 0, 0, 0, 0));
            }
            pop_bit(single_pushes, to_sq);
        }

        // double pushes
        U64 double_pushes = (pushes << 8) & 0x00000000FF000000ULL & ~pos->occupancy[BOTH];
        while (double_pushes) {
            to_sq = __builtin_ctzll(double_pushes);

            from_sq = to_sq - 16;
            addMove(list, encode_move(from_sq, to_sq, 0, 0, 1, 0));
            pop_bit(double_pushes, to_sq);
        }

        //captures 



        
    } else {
        bitboard = pos->pieces[P];

        pushes = (bitboard) >> 8 & ~pos->occupancy[BOTH]; // set move

        // single pushes
        U64 single_pushes = pushes;
        while (single_pushes) {
            to_sq = __builtin_ctzll(single_pushes);

            from_sq = to_sq + 8;

            if (to_sq >= A1 & to_sq <= H1) {
                addMove(list, encode_move(from_sq, to_sq, q, 0, 0, 0));
                addMove(list, encode_move(from_sq, to_sq, r, 0, 0, 0));
                addMove(list, encode_move(from_sq, to_sq, b, 0, 0, 0));
                addMove(list, encode_move(from_sq, to_sq, n, 0, 0, 0));
            } else {
                addMove(list, encode_move(from_sq, to_sq, 0, 0, 0, 0));
            }
            pop_bit(single_pushes, to_sq);
        }

        // double pushes on start
        U64 double_pushes = (pushes >> 8) & 0x000000FF00000000ULL & ~pos->occupancy[BOTH];
        while (double_pushes) {
            to_sq = __builtin_ctzll(double_pushes);

            from_sq = to_sq + 16;
            addMove(list, encode_move(from_sq, to_sq, 0, 0, 1, 0));
            pop_bit(double_pushes, to_sq);
        }

        // captures
    }
}