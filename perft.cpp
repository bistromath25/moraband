/**
 * Moraband, known in antiquity as Korriban, was an 
 * Outer Rim planet that was home to the ancient Sith 
 **/

#include "perft.h"
#include <numeric>
#include <thread>
#include <vector>

static History history;
std::vector<U64> nodeCount;

/** Perft test */
U64 perft(Position &s, int depth) {
    int nodes = 0;
    if (s.getFiftyMoveRule() > 99) return nodes;
    MoveList moveList(s);
    if (depth == 1) return moveList.size();
    Move m;
    while ((m = moveList.getBestMove())) {
        Position c(s);
        c.makeMove(m);
        nodes += perft(c, depth - 1);
    }
    return nodes;
}

void test(Position s, MoveList *moveList, int depth, int id) {
    Move m = moveList->getBestMove();
    while (m != NULL_MOVE) {
        Position c(s);
        c.makeMove(m);
        int nodes = perft(c, depth - 1);
        m = moveList->getBestMove();
        nodeCount.push_back(nodes);
    }
}

/** Multi-threaded Perft test */
U64 MTperft(Position &s, int depth) {
    std::vector<std::thread> threads;
    nodeCount.clear();
    MoveList moveList(s);
    for (unsigned int i = 0; i < NUM_THREADS; ++i) {
        threads.push_back(std::thread(test, s, &moveList, depth, i));
    }
    for (std::thread &thread : threads) thread.join();
    return std::accumulate(nodeCount.begin(), nodeCount.end(), 0);
}

void perftTest(Position &s, int depth, bool mt) {
    U64 nodes = 0;
    Clock clock;
    clock.set();
    nodes = mt ? MTperft(s, depth) : perft(s, depth);
    double time = clock.elapsed<std::chrono::microseconds>() / static_cast<double>(1000000);
    std::cout << s << std::endl;
    std::cout << s.getFen() << std::endl;
    std::cout << "Time:  " << time << std::endl;
    std::cout << "Nodes: " << nodes << std::endl;
    std::cout << "NPS:   " << U64(static_cast<long double>(nodes) / time) << std::endl;
}
