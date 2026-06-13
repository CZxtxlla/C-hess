#include "dataloader.h"

SparseBatch* create_sparse_batch(int batch_size) {
    SparseBatch* batch = (SparseBatch*)malloc(sizeof(SparseBatch));
    batch->batch_size = batch_size;

    // max 30 active features per position, because 32 pieces - 2 kings
    batch->max_nnz = batch_size * 30;
    batch->current_w_nnz = 0;
    batch->current_b_nnz = 0;

    batch->w_row_indices = (int*)malloc(batch->max_nnz * sizeof(int));
    batch->w_col_indices = (int*)malloc(batch->max_nnz * sizeof(int));
    batch->b_row_indices = (int*)malloc(batch->max_nnz * sizeof(int));
    batch->b_col_indices = (int*)malloc(batch->max_nnz * sizeof(int));
    batch->targets = (float*)malloc(batch_size * sizeof(float));

    return batch;
}

// helper to map piece encoding to 0-9 HalfKP piece type index
int get_halfkp_piece_type_white(uint8_t piece) {
    if (piece >= 1 && piece <= 5) return piece - 1; // 0 to 4 (friend)
    if (piece >= 7 && piece <= 11) return (piece - 7) + 5; // 5 to 9 (enemy)
    return -1; // empty or king
}

int get_halfkp_piece_type_black(uint8_t piece) {
    if (piece >= 7 && piece <= 11) return piece - 7; // 0 to 4 (friend)
    if (piece >= 1 && piece <= 5) return (piece - 1) + 5; // 5 to 9 (enemy)
    return -1; // empty or king
}

static inline int flip_sq(int sq) {
    return sq ^ 56;
}

// extract features and append them to batch arrays
void extract_features(ChessRecord* record, int current_batch_row, SparseBatch* batch) {
    int white_king_sq = -1;
    int black_king_sq = -1;

    // 1. Find both Kings
    for (int sq = 0; sq < 64; sq++) {
        if (record->board[sq] == 6) white_king_sq = sq;
        if (record->board[sq] == 12) black_king_sq = sq;
    }

    if (white_king_sq == -1 || black_king_sq == -1) return; // Invalid board

    // 2. Loop over the board and populate arrays
    for (int sq = 0; sq < 64; sq++) {
        uint8_t piece = record->board[sq];
        if (piece == 0 || piece == 6 || piece == 12) continue; // skip empty and kings

        // --- Extract White Perspective ---
        int w_p_type = get_halfkp_piece_type_white(piece);
        if (w_p_type != -1) {
            int w_feature_idx = white_king_sq * 640 + w_p_type * 64 + sq;
            
            int w_nnz = batch->current_w_nnz;
            batch->w_row_indices[w_nnz] = current_batch_row;
            batch->w_col_indices[w_nnz] = w_feature_idx;
            batch->current_w_nnz++;
        }

        // --- Extract Black Perspective ---
        int b_p_type = get_halfkp_piece_type_black(piece);
        if (b_p_type != -1) {
            // flip the squares so the network evaluates it identically
            int flipped_king_sq = flip_sq(black_king_sq);
            int flipped_piece_sq = flip_sq(sq);

            int b_feature_idx = flipped_king_sq * 640 + b_p_type * 64 + flipped_piece_sq;
            
            int b_nnz = batch->current_b_nnz;
            batch->b_row_indices[b_nnz] = current_batch_row;
            batch->b_col_indices[b_nnz] = b_feature_idx;
            batch->current_b_nnz++;
        }
    }
}

// returns 1 if full/partial batch loaded, 0 if end of the file
int load_next_batch(FILE* dataset_file, SparseBatch* batch) {
    batch->current_w_nnz = 0; // reset
    batch->current_b_nnz = 0;

    ChessRecord record;
    int items_read;

    for (int row = 0; row < batch->batch_size; row++) {
        items_read = fread(&record, sizeof(ChessRecord), 1, dataset_file);

        if (items_read == 0) {
            batch->batch_size = row;
            return (row > 0) ? 1 : 0;
        }

        batch->targets[row] = record.target;

        extract_features(&record, row, batch);
    }

    return 1;
}