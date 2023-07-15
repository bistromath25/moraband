#ifndef PERFT_H
#define PERFT_H


#include <ctime>
#include <string>
#include <iomanip>
#include "movegen.h"
#include "Position.h"
#include "defs.h"
#include "timeman.h"
#include "move.h"

U64 perft(Position & s, int depth);
void test(Position s, MoveList* moveList, int depth, int id);
U64 MTperft(Position& s, int depth);
void perftTest(Position& s, int depth, bool mt);

#endif