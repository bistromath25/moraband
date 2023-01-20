#ifndef MOVE_GENERATOR_H
#define MOVE_GENERATOR_H

#include <cmath>
#include <algorithm>
#include <vector>
#include <array>
#include "state.h"
#include "board.h"
#include "MagicMoves.hpp"
#include "defs.h"
#include "move.h"
#include "history.h"

const U64 Full = 0xFFFFFFFFFFFFFFFF;
const int maxSize = 256;

enum MoveStage {
	nBestMove,
	nAttacksGen,
	nAttacks,
	nKiller1,
	nKiller2,
	nQuietsGen,
	nQuiets,
	qBestMove,
	qAttacksGen,
	qAttacks,
	qQuietChecksGen,
	qQuietChecks,
	nEvadeBestMove,
	nEvadeMovesGen,
	nEvade,
	allLegal
};

enum class MoveType {
	Attacks,
	Quiets,
	Evasions,
	QuietChecks,
	All
};

class MoveList {
public:
	MoveList(const State& pState, Move pBest, History* pHistory, int pPly, bool pQSearch=false);
	MoveList(const State& pState);
	std::size_t size() const;
	void push(Move m);
	bool contains(Move move) const;
	Move getBestMove();
	//Move getKiller1() const;
	//Move getKiller2() const;
	//void setKiller1(Move m);
	//void setKiller2(Move m);
	Move pop();
	void checkLegal();
	void setStage(int stage) { mStage = stage; }
	
	template<MoveType T> void generateMoves();
	template<MoveType T, Color C> void pushPawnMoves();
	template<MoveType T, PieceType P> void pushMoves();
	template<MoveType T> void pushCastle();
	template<MoveType T> void pushPromotion(Square src, Square dst);
	friend std::ostream & operator << (std::ostream & os, const MoveList & mlist);

private:
	bool mQSearch;
	U64 mValid;
	U64 mDiscover;
	const State& mState;
	const History* mHistory;
	int mPly;
	int mStage;
	std::array<MoveEntry, maxSize> mList;
	std::vector<MoveEntry> badCaptures;
	std::size_t mSize;
	Move mBest;
	Move mKiller1;
	Move mKiller2;
};

void mg_init();

#endif

///