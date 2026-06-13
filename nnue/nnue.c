#include "nnue.h"
#include "../include/position.h"
#include <string.h>

NNUE_Network global_nnue;

void init_nnue(const char* filepath) {
    FILE* f = fopen(filepath, "rb");
    if (!f) {
        printf("Error: could not open nnue file at %s\n", filepath);
        exit(1);
    }

    size_t read_items = fread(&global_nnue, sizeof(NNUE_Network), 1, f);
    if (read_items != 1) {
        printf("Error: Failed to read complete NNUE network\n");
        exit(1);
    }

    fclose(f);
    printf("NNUE loaded succesffuly.\n");
}

// turns board piece into index for halfkp between 0 and 40959
int get_feature_index(int is_white, int king_sq, int piece_type, int piece_sq) {
    if (!is_white) {
        king_sq ^= 56;
        piece_sq ^= 56;
    }

    return king_sq * 640 + piece_type * 64 + piece_sq;
}

// update accumulator when piece placed on board
void add_feature(Accumulator* acc, int feature_idx) {
    for (int i = 0; i < L1_SIZE; i++) {
        acc->values[i] += global_nnue.feature_weights[feature_idx][i];
    }
}

// update accumulator when piece removed from board
void remove_feature(Accumulator* acc, int feature_idx) {
    for (int i = 0; i < L1_SIZE; i++) {
        acc->values[i] -= global_nnue.feature_weights[feature_idx][i];
    }
}

// Convert engine piece types to halfkp piece types
int get_halfkp_piece(int pt, int is_white_perspective) {
    // Check boundaries using your enums
    int is_white_piece = (pt >= P && pt <= Q);
    int is_black_piece = (pt >= p && pt <= q);
    
    if (is_white_perspective) {
        if (is_white_piece) return pt - P;         // Maps P..Q to 0..4
        if (is_black_piece) return (pt - p) + 5;   // Maps p..q to 5..9
    } else {
        if (is_black_piece) return pt - p;         // Maps p..q to 0..4
        if (is_white_piece) return (pt - P) + 5;   // Maps P..Q to 5..9
    }
    
    return -1; // Fallback for Kings or empty squares
}

void refresh_accumulator(Position* pos, Accumulator* w_acc, Accumulator* b_acc) {
    // Reset both accumulators to the base bias
    for (int i = 0; i < L1_SIZE; i++) {
        w_acc->values[i] = global_nnue.feature_bias[i];
        b_acc->values[i] = global_nnue.feature_bias[i];
    }

    // Extract the King squares from the bitboards
    int white_king_sq = __builtin_ctzll(pos->pieces[K]);
    int black_king_sq = __builtin_ctzll(pos->pieces[k]);

    // 3. Loop over all piece types
    for (int pt = P; pt <= k; pt++) {
        if (pt == K || pt == k) continue; // Skip Kings

        U64 bitboard = pos->pieces[pt];
        
        // Get the mapped piece types (0-9) for both perspectives
        int w_pt = get_halfkp_piece(pt, 1); // 1 = White Perspective
        int b_pt = get_halfkp_piece(pt, 0); // 0 = Black Perspective
        
        if (w_pt == -1 || b_pt == -1) continue; // Skip empty/invalid types

        while (bitboard) {
            int sq = __builtin_ctzll(bitboard);
            
            // Get the 0-40959 index
            // 1 = White, 0 = Black
            int w_idx = get_feature_index(1, white_king_sq, w_pt, sq);
            int b_idx = get_feature_index(0, black_king_sq, b_pt, sq);

            // Add the weights directly to the accumulators
            for (int i = 0; i < L1_SIZE; i++) {
                w_acc->values[i] += global_nnue.feature_weights[w_idx][i];
                b_acc->values[i] += global_nnue.feature_weights[b_idx][i];
            }
            
            bitboard &= bitboard - 1; // Clear the lowest set bit
        }
    }
}


// Returns the evaluation in Centipawns relative to White
int evaluate_nnue(Accumulator* w_acc, Accumulator* b_acc) {
    int32_t l1_input[512]; 
    
    // 1. Concatenate accumulators and apply Clipped ReLU
    for(int i = 0; i < L1_SIZE; i++) {
        l1_input[i] = (w_acc->values[i] < 0) ? 0 : ((w_acc->values[i] > 255) ? 255 : w_acc->values[i]);
        l1_input[i + L1_SIZE] = (b_acc->values[i] < 0) ? 0 : ((b_acc->values[i] > 255) ? 255 : b_acc->values[i]);
    }

    // 2. Layer 1 (512 -> 32)
    int32_t l1_out[32];
    for(int i = 0; i < 32; i++) l1_out[i] = global_nnue.l1_bias[i] * 255; // Base bias
    
    for(int j = 0; j < 512; j++) {
        // --- SPARSITY OPTIMIZATION ---
        // If the neuron is 0, skip all 32 multiplications!
        if (l1_input[j] == 0) continue; 
        
        for(int i = 0; i < 32; i++) {
            // Memory is now accessed sequentially, eliminating CPU cache misses
            l1_out[i] += l1_input[j] * global_nnue.l1_weights[j][i];
        }
    }
    
    for(int i = 0; i < 32; i++) {
        l1_out[i] /= 255;
        l1_out[i] = (l1_out[i] < 0) ? 0 : ((l1_out[i] > 255) ? 255 : l1_out[i]); // ReLU
    }

    // 3. Layer 2 (32 -> 32)
    int32_t l2_out[32];
    for(int i = 0; i < 32; i++) l2_out[i] = global_nnue.l2_bias[i] * 255;
    
    for(int j = 0; j < 32; j++) {
        if (l1_out[j] == 0) continue; // Skip dead neurons
        for(int i = 0; i < 32; i++) {
            l2_out[i] += l1_out[j] * global_nnue.l2_weights[j][i];
        }
    }
    
    for(int i = 0; i < 32; i++) {
        l2_out[i] /= 255;
        l2_out[i] = (l2_out[i] < 0) ? 0 : ((l2_out[i] > 255) ? 255 : l2_out[i]); // ReLU
    }

    // 4. Output Layer (32 -> 1)
    int32_t output = global_nnue.output_bias[0] * 255;
    for(int j = 0; j < 32; j++) {
        if (l2_out[j] == 0) continue;
        output += l2_out[j] * global_nnue.output_weight[j][0];
    }

    // 5. Convert to Centipawns
    return (int)(((long long) output * 400) /65025); 
}