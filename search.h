#ifndef SEARCH_H
#define SEARCH_H

#include <thread>

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
#include "timeman.h"
#include "variation.h"
#include "move.h"
#include "history.h"
//#include "book.h"

static const int LMR_COUNT = 3;
static const int LMR_DEPTH = 2;
static const int NULL_MOVE_COUNT = 3;
static const int NULL_MOVE_DEPTH = 3;
static const int NULL_MOVE_MARGIN = 100; // NMP pruning margin
static const int REVERSE_FUTILITY_DEPTH = 3;
static const int REVERSE_FUTILITY_MARGIN = 200;
static const int TT_REDUCTION_DEPTH = 4;
static const int ASP_DELTA[] = { 10, 20, 40, 100, 200, 400, POS_INF };

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
	bool quit, infinite;
	std::vector<Move> sm;
};

struct GlobalInfo {
	GlobalInfo() {
		nodes = 0;
		history.clear();
		variation.clearPv();
	}
	U64 nodes;
	History history;
	Variation variation;
};

static const int MAX_THREADS = 16;
//static int NUM_THREADS = 1;
static bool THREAD_STOP = false;
static std::thread threads[MAX_THREADS];
static GlobalInfo global_info[MAX_THREADS];
static std::pair<int, bool> results[MAX_THREADS];

int search(State& s, SearchInfo& si, GlobalInfo& gi, int depth, int ply, int alpha, int beta, bool isPv, bool isNull, bool isRoot);
int search_root(State& s, SearchInfo& si, GlobalInfo& gi, int depth, int ply, int alpha, int beta);
Move iterative_deepening(State& s, SearchInfo& si, int NUM_THREADS);
Move search(State& s, SearchInfo& si, int NUM_THREADS);

#endif  