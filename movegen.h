/**
 * Moraband, known in antiquity as Korriban, was an 
 * Outer Rim planet that was home to the ancient Sith 
 **/

#ifndef MOVE_GENERATOR_H
#define MOVE_GENERATOR_H

#include <cmath>
#include <algorithm>
#include <vector>
#include <array>
#include "position.h"
#include "board.h"
#include "MagicMoves.hpp"
#include "defs.h"
#include "move.h"
#include "history.h"

const U64 FULL = 0xFFFFFFFFFFFFFFFF;
const int MOVELIST_MAX_SIZE = 256;

/* Staged move generation */
enum MoveStage {
	BestMove,
	AttacksGen,
	Attacks,
	Killer1,
	Killer2,
	QuietsGen,
	Quiets,
	QBestMove,
	QAttacksGen,
	QAttacks,
	QQuietChecksGen,
	QQuietChecks,
	EvadeBestMove,
	EvadeMovesGen,
	Evade,
	AllLegal
};

/* Types of moves */
enum class MoveType {
	Attacks,
	Quiets,
	Evasions,
	QuietChecks,
	All
};

/* List of moves and related functions */
class MoveList {
public:
	MoveList(const Position& s, Move bestMove, History* history, int ply, bool qsearch=false);
	MoveList(const Position& s);
	std::size_t size() const;
	void push(Move m);
	bool contains(Move move) const;
	Move getBestMove();
	Move pop();
	void checkLegal();
	void setStage(int stage) { stage = stage; }
	
	template<MoveType T> void generateMoves();
	template<MoveType T, Color C> void pushPawnMoves();
	template<MoveType T, PieceType P> void pushMoves();
	template<MoveType T> void pushCastle();
	template<MoveType T> void pushPromotion(Square src, Square dst);
	friend std::ostream & operator << (std::ostream & os, const MoveList & moveList);

private:
	bool isQSearch;
	U64 valid;
	U64 discover;
	const Position& position;
	const History* history;
	int ply;
	int stage;
	std::array<MoveEntry, MOVELIST_MAX_SIZE> moveList;
	std::vector<MoveEntry> badCaptures;
	std::size_t sz;
	Move best;
	Move killer1;
	Move killer2;
};

void mg_init();

#endif