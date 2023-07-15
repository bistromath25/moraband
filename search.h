/**
 * Moraband, known in antiquity as Korriban, was an 
 * Outer Rim planet that was home to the ancient Sith 
 **/

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
#include "Position.h"
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
static const int RAZORING_DEPTH = 2;
static const int RAZORING_MARGIN[] = { 0, 200, 400 };
static const int TT_REDUCTION_DEPTH = 4;
static const int ASP_DELTA[] = { 10, 20, 40, 100, 200, 400, POS_INF };

inline int value_to_tt(int value, int ply) {
	if (value >= CHECKMATE_BOUND) {
		value += ply;
	}
	else if (value <= -CHECKMATE_BOUND) {
		value -= ply;
	}
	return value;
}

inline int value_from_tt(int value, int ply) {
	if (value >= CHECKMATE_BOUND) {
		value -= ply;
	}
	else if (value <= -CHECKMATE_BOUND) {
		value += ply;
	}
	return value;
}

enum SearchType {
	qSearch,
	scoutSearch
};

struct SearchInfo {
	SearchInfo() : moveTime(0), nodes(0), prevNodes(0), totalNodes(0), moves_to_go(0), quit(false), infinite(false), depth(MAX_PLY), time{}, inc{} {}
	int time[PLAYER_SIZE], inc[PLAYER_SIZE];
	int moves_to_go, depth, max_nodes, nodes, prevNodes;
	U64 totalNodes;
	U64 moveTime;
	Clock clock;
	bool quit, infinite;
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

const int MAX_THREADS = 16;
extern int NUM_THREADS;
extern bool THREAD_STOP;
extern std::thread threads[MAX_THREADS];
extern GlobalInfo global_info[MAX_THREADS];
extern std::pair<int, bool> results[MAX_THREADS];

int qsearch(Position& s, SearchInfo& si, GlobalInfo& gi, int ply, int alpha, int beta);
int search(Position& s, SearchInfo& si, GlobalInfo& gi, int depth, int ply, int alpha, int beta, bool isPv, bool isNull, bool isRoot);
int search_root(Position& s, SearchInfo& si, GlobalInfo& gi, int depth, int ply, int alpha, int beta);
Move iterative_deepening(Position& s, SearchInfo& si);
Move search(Position& s, SearchInfo& si);

#endif