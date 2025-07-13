/**
 * Moraband, known in antiquity as Korriban, was an 
 * Outer Rim planet that was home to the ancient Sith 
 **/

#ifndef SEARCH_H
#define SEARCH_H

#include <thread>

#include "defs.h"
#include "eval.h"
#include "history.h"
#include "move.h"
#include "movegen.h"
#include "position.h"
#include "timeman.h"
#include "tt.h"
#include "variation.h"
#include <algorithm>
#include <atomic>
#include <sstream>
#include <string>
#include <vector>

constexpr int LMR_COUNT = 3;
constexpr int LMR_DEPTH = 2;
constexpr int NULL_MOVE_COUNT = 3;
constexpr int NULL_MOVE_DEPTH = 4;
constexpr int NULL_MOVE_MARGIN = 100; // NMP pruning margin
constexpr int REVERSE_FUTILITY_DEPTH = 2;
constexpr int REVERSE_FUTILITY_MARGIN = 200;
constexpr int FUTILITY_DEPTH = 7;
constexpr int RAZOR_DEPTH = 2;
constexpr int RAZOR_MARGIN = 300;
constexpr int LATE_MOVE_REDUCTION_DEPTH = 3;

struct SearchInfo {
    SearchInfo() : time{}, inc{}, moves_to_go(0), depth(MAX_PLY), nodes(0), prevNodes(0), totalNodes(0), moveTime(0), quit(false), infinite(false) {}
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
        std::fill(evalHistory.begin(), evalHistory.end(), 0);
    }
    void init() {
        clear();
        std::fill(evalHistory.begin(), evalHistory.end(), 0);
    }
    void clear() {
        nodes = 0;
        history.clear();
        variation.clearPv();
    }
    U64 nodes;
    History history;
    Variation variation;
    std::array<U64, 64> evalHistory;
};

const int MAX_THREADS = 16;
extern int NUM_THREADS;
extern std::atomic<bool> THREAD_STOP;
extern std::thread threads[MAX_THREADS];
extern GlobalInfo global_info[MAX_THREADS];
extern std::pair<int, bool> results[MAX_THREADS];

int qsearch(Position &s, SearchInfo &si, GlobalInfo &gi, int ply, int alpha, int beta);
int search(Position &s, SearchInfo &si, GlobalInfo &gi, int depth, int ply, int alpha, int beta, bool isPv, bool isNull);
int search_root(Position &s, SearchInfo &si, GlobalInfo &gi, int depth, int ply, int alpha, int beta);
Move iterative_deepening(Position &s, SearchInfo &si);
Move search(Position &s, SearchInfo &si);

#endif
