#ifndef SEARCH_H
#define SEARCH_H

#include "position.h"

// recursively perform alpha beta pruning 
int negamax(Position* pos, int depth, int distance, int alpha, int beta); 

// function to call negamax and format the output
void search_position(Position* pos, int depth);

// base case for negamax that continues until there are no captures
int quiescence(Position* pos, int alpha, int beta);



#endif