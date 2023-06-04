/**
 * Moraband, known in antiquity as Korriban, was an 
 * Outer Rim planet that was home to the ancient Sith 
 **/

#ifndef HISTORY_H
#define HISTORY_H

#include <iostream>
#include <vector>
#include <algorithm>
#include "state.h"
#include "move.h"
#include "defs.h"
#include "state.h"

/* History heuristic */
class History {
public:
	History() : mKillers{}, mHistory{} {
		mGameHistory.reserve(1024);
		for (std::array<int, BOARD_SIZE>& arr : mButterfly) {
			arr.fill(1);
		}
	}
	History(const State& s) : mKillers{}, mHistory{} {
		mGameHistory.reserve(1024);
		for (std::array<int, BOARD_SIZE>& arr : mButterfly) {
			arr.fill(1);
		}
		push(std::make_pair(NULL_MOVE, s.getKey()));
	}
	std::size_t size() {
		return mGameHistory.size();
	}
	void init() {
		mKillers = {};
		mHistory = {};
		mGameHistory.clear();
		for (std::array<int, BOARD_SIZE>& arr : mButterfly) {
			arr.fill(1);
		}
	}
	void clear() {
		mKillers = {};
		mHistory = {};
		for (std::array<int, BOARD_SIZE>& arr : mButterfly) {
			arr.fill(1);
		}
	}
	void push(const std::pair<Move, U64>& p) {
		mGameHistory.push_back(p);
	}
	void pop() {
		if (!mGameHistory.empty()) {
			mGameHistory.pop_back();
		}
	}
	bool isThreefoldRepetition(State& s) const {
		int cnt = 0;
		for (int i = mGameHistory.size() - 5; i >= 0; i -= 4) {
			if (mGameHistory[i].second == s.getKey() && ++cnt == 2) {
				return true;
			}
		}
		return false;
	}
	void update(Move bestMove, int depth, int ply, bool causedCutoff) {
		if (causedCutoff) {
			if (mKillers[ply].first != bestMove) {
				mKillers[ply].second = mKillers[ply].first;
				mKillers[ply].first = bestMove;
			}
			mHistory[getSrc(bestMove)][getDst(bestMove)] += depth * depth;
		}
		else {
			mButterfly[getSrc(bestMove)][getDst(bestMove)] += depth;
		}
	}
	const std::pair<Move, Move>& getKiller(int ply) const {
		return mKillers[ply];
	}
	int getHistoryScore(Move move) const {
		assert(mButterfly[getSrc(move)][getDst(move)] > 0);
		return mHistory[getSrc(move)][getDst(move)] / mButterfly[getSrc(move)][getDst(move)];
	}
	int count(State& s) {
		int cnt = 0;
		for (int i = mGameHistory.size() - 1; i >= 0; i -= 4) {
			if (mGameHistory[i].second == s.getKey()) {
				cnt++;
			}
		}
		return cnt;
	}
private:
	std::vector<std::pair<Move, U64>> mGameHistory;
	std::array<std::pair<Move, Move>, MAX_PLY> mKillers;
	std::array<std::array<int, BOARD_SIZE>, BOARD_SIZE> mButterfly;
	std::array<std::array<int, BOARD_SIZE>, BOARD_SIZE> mHistory;
};

#endif