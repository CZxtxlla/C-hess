#include "../include/bitboard.h"
#include "../include/magic.h"
#include "../include/position.h"
#include "../include/uci.h"
#include "../include/zobrist.h"
#include "../nnue/nnue.h"

int main() {
    init_leapers();
    init_sliders();
    
    init_zobrist();
    init_tt(64); // 64 megabytes

    init_nnue("halfkp_model_quant.nnue");
    Position board;
    
    // Hand control over to the UCI listener
    uci_loop(&board);

    return 0;
}