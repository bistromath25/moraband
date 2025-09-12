/**
 * Moraband, known in antiquity as Korriban, was an 
 * Outer Rim planet that was home to the ancient Sith 
 **/

#ifndef SEARCH_H
#define SEARCH_H

#include "defs.h"
#include "history.h"
#include "move.h"
#include "position.h"
#include "timeman.h"
#include "variation.h"

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
constexpr int ASPIRATION_WINDOW = 30;

/** Search information */
struct SearchInfo {
    SearchInfo() : time{}, inc{}, movesToGo(0), depth(MAX_PLY), nodes(0), prevNodes(0), maxNodes(0), totalNodes(0), moveTime(0), quit(false), infinite(false) {}
    int time[PLAYER_SIZE], inc[PLAYER_SIZE];
    int movesToGo, depth, nodes, prevNodes;
    U64 maxNodes, totalNodes;
    U64 moveTime;
    Clock clock;
    bool quit, infinite;
};

/** Global search information */
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
    std::array<int, 64> evalHistory;
};

constexpr int MAX_THREADS = 64;
extern int NUM_THREADS;
extern GlobalInfo global_info[MAX_THREADS];

#ifdef TUNE
int qsearch(Position &s, SearchInfo &si, GlobalInfo &gi, int ply, int alpha, int beta);
#endif
Move search(Position &s, SearchInfo &si);

#endif
