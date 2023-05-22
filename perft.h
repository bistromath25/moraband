#ifndef PERFT_H
#define PERFT_H


#include <ctime>
#include <string>
#include <iomanip>
#include "movegen.h"
#include "state.h"
#include "defs.h"
#include "timeman.h"
#include "move.h"

U64 perft(State & s, int depth);
void test(State s, MoveList* moveList, int depth, int id);
U64 MTperft(State& s, int depth);
void perftTest(State& s, int depth, bool mt);

#endif