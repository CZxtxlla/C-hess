#ifndef DATALOADER_H
#define DATALOADER_H

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>

// A highly compact binary representation of a single position
// Size: 64 bytes (pieces) + 4 bytes (target) = 68 bytes per game state
typedef struct __attribute__((packed)) {
    // Array of 64 squares. 
    // e.g., 0 = Empty, 1 = White Pawn, ..., 6 = White King, 7 = Black Pawn, etc.
    uint8_t board[64]; 
    
    // The training target (e.g., 1.0 for White Win, 0.0 for Black Win, 0.5 for Draw)
    float target;      
} ChessRecord;

// Holds a single batch of sparse data
typedef struct {
    int batch_size;
    int max_nnz; // Maximum possible Non-Zero elements allocated

    int current_w_nnz; // How many non-zero white perspective elements actually exist in this batch
    int current_b_nnz; // How many non-zero black perspective elements actually exist in this batch
    
    // Sparse Coordinate Arrays (COO format)
    int* w_row_indices; // The index in the batch (0 to batch_size - 1)
    int* w_col_indices; // The HalfKP feature index (0 to 40959)

    int* b_row_indices; // The index in the batch (0 to batch_size - 1)
    int* b_col_indices; // The HalfKP feature index (0 to 40959)
    
    // Dense array for the targets
    float* targets;
} SparseBatch;

// allocate the batch once at startup
SparseBatch* create_sparse_batch(int batch_size); 

// load data into sparsebatch
int load_next_batch(FILE* dataset_file, SparseBatch* batch);

#endif