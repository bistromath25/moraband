#ifndef SEARCH_H
#define SEARCH_H

#include <algorithm>
#include <stack>
#include <sstream>
#include <string>
#include <vector>
#include "eval.h"
#include "movegen.h"
#include "state.h"
#include "defs.h"
#include "tt.h"
#include "timer.h"
#include "variation.h"
#include "move.h"
#include "history.h"
#include "book.h"

static const int LMR_COUNT = 3;
static const int LMR_DEPTH = 2;
static const int NULL_MOVE_COUNT = 3;
static const int NULL_MOVE_DEPTH = 3;

enum SearchType {
	qSearch,
	scoutSearch
};

struct SearchInfo {
	SearchInfo() : moveTime(0), nodes(0), prevNodes(0), moves_to_go(0), quit(false), infinite(false), depth(MAX_PLY), time{}, inc{} {}
	int time[PLAYER_SIZE], inc[PLAYER_SIZE];
	int moves_to_go, depth, max_nodes, nodes, prevNodes, mate;
	int64_t moveTime;
	Clock clock;
	bool infinite, quit;
	std::vector<Move> sm;
};

extern History history;
extern Variation variation;
void setup_search(State& s, SearchInfo& si);
void iterative_deepening(State& s, SearchInfo& si);
int scout_search(State& s, SearchInfo& si, int depth, int ply, int alpha, int beta, bool isPv, bool isNull, bool isRoot);

#endif  