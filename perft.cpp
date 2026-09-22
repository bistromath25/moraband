/**
 * Moraband, known in antiquity as Korriban, was an 
 * Outer Rim planet that was home to the ancient Sith 
 **/

#include "perft.h"
#include "search.h"
#include <numeric>
#include <thread>
#include <vector>

/** Perft test */
U64 perft(Position &s, int depth) {
    if (s.getFiftyMoveRule() > 99) {
        return 0;
    }
    MoveList moveList(s);
    if (depth == 1) {
        return moveList.size();
    }
    U64 nodes = 0;
    while (Move m = moveList.getBestMove()) {
        StateInfo st;
        s.makeMove(m, st);
        nodes += perft(s, depth - 1);
        s.undoMove(m, st);
    }
    return nodes;
}

void perftWorker(const Position &pos, const std::vector<Move> &moves, int depth, size_t start,
                 size_t step, std::vector<U64> &results) {
    Position c(pos);
    for (size_t i = start; i < moves.size(); i += step) {
        StateInfo st;
        c.makeMove(moves[i], st);
        results[i] = perft(c, depth - 1);
        c.undoMove(moves[i], st);
    }
}

/** Multi-threaded Perft test */
U64 MTperft(const Position &pos, int depth) {
    MoveList moveList(pos);
    std::vector<Move> moves;
    while (Move m = moveList.getBestMove()) {
        moves.push_back(m);
    }
    std::vector<U64> results(moves.size(), 0);
    std::vector<std::thread> threads;
    int numThreads = std::min(NUM_THREADS, static_cast<int>(moves.size()));
    for (int t = 0; t < numThreads; ++t) {
        threads.emplace_back(perftWorker, std::cref(pos), std::cref(moves),
                             depth, t, numThreads, std::ref(results));
    }
    for (auto &thread : threads) {
        thread.join();
    }
    return std::accumulate(results.begin(), results.end(), 0ULL);
}

void perftTest(const Position &s, int depth, bool mt) {
    U64 nodes = 0;
    Clock clock;
    clock.set();
    if (mt) {
        nodes = MTperft(s, depth);
    }
    else {
        Position c(s);
        nodes = perft(c, depth);
    }
    double time = clock.elapsed<std::chrono::microseconds>() / static_cast<double>(1000000);
    std::cout << s << std::endl;
    std::cout << s.getFen() << std::endl;
    std::cout << "Time:  " << time << std::endl;
    std::cout << "Nodes: " << nodes << std::endl;
    std::cout << "NPS:   " << U64(static_cast<long double>(nodes) / time) << std::endl;
}
