#ifndef PERFT_H
#define PERFT_H


#include <ctime>
#include <string>
#include <iomanip>
#include "movegen.h"
#include "state.h"
#include "defs.h"
#include "timer.h"
#include "move.h"

int perft(State & s, int depth);
void test(State s, MoveList* mList, int depth, int id);
int MTperft(State& s, int depth);
void perftTest(State& s, int depth);

#endif