/* movegen.h */

#ifndef MOVEGEN_H
#define MOVEGEN_H

#include <cmath>
#include <algorithm>
#include <vector>
#include <array>

#include "bitboard.h"
#include "move.h"
#include "magicmoves.h"
#include "defs.h"

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
	MoveList()
}

// search
enum Phase {
	pawn_phase = 0;
	knight_phase = 1;
	bishop_phase = 1;
	rook_phase = 2;
	queen_phase = 4;
	total_phase = 24;
};

class State {
public:
	State() {};
	State(const std::string &);
	State(const State &s);
	void operator = (const State &);
	void init();
private:
	Color us;
	Color them;
	int fifty_moves;
	int castling_rights;
	int phase;
	U64 key;
	U64 pawn_key;
	U64 checkers;
	U64 attackers;
	U64 en_passant;
	std::array<U64, PIECE_TYPES_SIZE> check_squares;
	std::array<U64, PLAYER_SIZE> pinned;
	std::array<U64, PLAYER_SIZE> occupancy;
	std::array<int, BOARD_SIZE> piece_index;
};




#endif
