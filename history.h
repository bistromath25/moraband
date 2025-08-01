/**
 * Moraband, known in antiquity as Korriban, was an 
 * Outer Rim planet that was home to the ancient Sith 
 **/

#ifndef HISTORY_H
#define HISTORY_H

#include "defs.h"
#include "move.h"
#include "position.h"
#include <algorithm>
#include <iostream>
#include <vector>

/** History heuristic for move ordering */
class History {
public:
    History() : killers{}, history{} {
        game_history.reserve(1024);
        for (std::array<int, BOARD_SIZE> &b : butterfly) {
            b.fill(1);
        }
    }
    History(const Position &s) : killers{}, history{} {
        game_history.reserve(1024);
        for (std::array<int, BOARD_SIZE> &b : butterfly) {
            b.fill(1);
        }
        push(std::make_pair(NULL_MOVE, s.getKey()));
    }
    std::size_t size() {
        return game_history.size();
    }
    void init() {
        killers = {};
        history = {};
        game_history.clear();
        for (std::array<int, BOARD_SIZE> &b : butterfly) {
            b.fill(1);
        }
    }
    void clear() {
        killers = {};
        history = {};
        for (std::array<int, BOARD_SIZE> &b : butterfly) {
            b.fill(1);
        }
    }
    void push(const std::pair<Move, U64> &p) {
        game_history.push_back(p);
    }
    void pop() {
        if (!game_history.empty()) {
            game_history.pop_back();
        }
    }
    bool isThreefoldRepetition(Position &s) const {
        int cnt = 0;
        for (int i = game_history.size() - 5; i >= 0; i -= 4) {
            if (game_history[i].second == s.getKey() && ++cnt == 2) {
                return true;
            }
        }
        return false;
    }
    void update(Move bestMove, int depth, int ply, bool causedCutoff) {
        if (causedCutoff) {
            if (killers[ply].first != bestMove) {
                killers[ply].second = killers[ply].first;
                killers[ply].first = bestMove;
            }
            history[getSrc(bestMove)][getDst(bestMove)] += depth * depth;
        }
        else {
            butterfly[getSrc(bestMove)][getDst(bestMove)] += depth;
        }
    }
    const std::pair<Move, Move> &getKiller(int ply) const {
        return killers[ply];
    }
    int getHistoryScore(Move move) const {
        assert(butterfly[getSrc(move)][getDst(move)] > 0);
        return history[getSrc(move)][getDst(move)] / butterfly[getSrc(move)][getDst(move)];
    }
    int count(Position &s) {
        int cnt = 0;
        for (int i = game_history.size() - 1; i >= 0; i -= 4) {
            if (game_history[i].second == s.getKey()) {
                cnt++;
            }
        }
        return cnt;
    }

private:
    std::vector<std::pair<Move, U64>> game_history;
    std::array<std::pair<Move, Move>, MAX_PLY> killers;
    std::array<std::array<int, BOARD_SIZE>, BOARD_SIZE> butterfly;
    std::array<std::array<int, BOARD_SIZE>, BOARD_SIZE> history;
};

#endif
