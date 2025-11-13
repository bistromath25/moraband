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
U64 perft(const Position &s, int depth) {
    int nodes = 0;
    if (s.getFiftyMoveRule() > 99) {
        return nodes;
    }
    MoveList moveList(s);
    if (depth == 1) {
        return moveList.size();
    }
    while (Move m = moveList.getBestMove()) {
        Position c(s);
        c.makeMove(m);
        nodes += perft(c, depth - 1);
    }
    return nodes;
}

void perftWorker(const Position &pos, const std::vector<Move> &moves, int depth, size_t start,
                 size_t step, std::vector<U64> &results) {
    for (size_t i = start; i < moves.size(); i += step) {
        Position child(pos);
        child.makeMove(moves[i]);
        results[i] = perft(child, depth - 1);
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
    nodes = mt ? MTperft(s, depth) : perft(s, depth);
    double time = clock.elapsed<std::chrono::microseconds>() / static_cast<double>(1000000);
    std::cout << s << std::endl;
    std::cout << s.getFen() << std::endl;
    std::cout << "Time:  " << time << std::endl;
    std::cout << "Nodes: " << nodes << std::endl;
    std::cout << "NPS:   " << U64(static_cast<long double>(nodes) / time) << std::endl;
}
