#ifndef PERFT_H
#define PERFT_H

#include "defs.h"
#include "move.h"
#include "movegen.h"
#include "position.h"
#include "timeman.h"
#include <ctime>
#include <iomanip>
#include <string>

U64 perft(Position &s, int depth);
void test(Position s, MoveList *moveList, int depth, int id);
U64 MTperft(Position &s, int depth);
void perftTest(Position &s, int depth, bool mt);

#endif