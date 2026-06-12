#ifndef NNUE_H
#define NNUE_H

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#define FEATURE_SIZE 40960
#define L1_SIZE 256


// This holds the global weights loaded from the .bin file
typedef struct {
    int16_t feature_weights[FEATURE_SIZE][L1_SIZE];
    int16_t feature_bias[L1_SIZE];
    // Add weights/biases for remaining layers here
} NNUE_Network;

// This holds the running state of the first layer
typedef struct {
    int16_t values[L1_SIZE];
} Accumulator;

NNUE_Network global_nnue; // global network

// take in file with weights and biases and 
void init_nnue(const char* filepath);

// turns board piece into index for halfkp between 0 and 40959
int get_feature_index(int is_white, int king_sq, int piece_type, int piece_sq);

// update accumulator when piece placed on board
void add_feature(Accumulator* acc, int feature_idx);

// update accumulator when piece removed from board
void remove_feature(Accumulator* acc, int feature_idx);

// refresh accumulator to starting position
void refresh_accumulator(Accumulator* acc);




#endif