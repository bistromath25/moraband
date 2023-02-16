#ifndef EVAL_H
#define EVAL_H

#include <iostream>
#include <iomanip>
#include <array>
#include <algorithm>
#include "state.h"
#include "pst.h"

static const int TEMPO_BONUS = 8; // Side-to-move bonus
static int CONTEMPT = 0; // Contempt

static const int KNIGHT_THREAT = 2; // Threats on enemy King
static const int BISHOP_THREAT = 2;
static const int ROOK_THREAT = 3;
static const int QUEEN_THREAT = 5;

static const int PAWN_WEIGHT_MG = 82;
static const int KNIGHT_WEIGHT_MG = 337;
static const int BISHOP_WEIGHT_MG = 365;
static const int ROOK_WEIGHT_MG = 477;
static const int QUEEN_WEIGHT_MG = 1025;

static const int PAWN_WEIGHT_EG = 94;
static const int KNIGHT_WEIGHT_EG = 281;
static const int BISHOP_WEIGHT_EG = 297;
static const int ROOK_WEIGHT_EG = 512;
static const int QUEEN_WEIGHT_EG = 936;

// Pawn eval
static const int PAWN_PASSED = 20;
static const int PAWN_PASSED_CANDIDATE = 10;
static const int PAWN_CONNECTED = 10;
static const int PAWN_ISOLATED = -10;
static const int PAWN_DOUBLED = -5;
static const int PAWN_FULL_BACKWARDS = -15;
static const int PAWN_BACKWARDS = -5;
static const int BAD_BISHOP = -2;
static const int TRAPPED_ROOK = -25;
static const int STRONG_PAWN_ATTACK = 80; // -80
static const int WEAK_PAWN_ATTACK = 40; // -40
static const int HANGING = -25;

// Assorted bonuses (maybe not used)
/*
static const int BISHOP_PAIR_MG = 7;
static const int BISHOP_PAIR_EG = 10;
static const int ROOK_OPEN_FILE_MG = 7;
static const int ROOK_OPEN_FILE_EG = 8;
static const int OUTPOST_BONUS = 5;
*/

static const int CHECKMATE = 32767;
static const int MAX_CHECKMATE = CHECKMATE + 2000;
static const int CHECKMATE_BOUND = CHECKMATE - MAX_PLY;
static const int STALEMATE = 0;
static const int DRAW = 0;

// Game phase calculations
static const int MIDDLEGAME_LIMIT = 4500;
static const int ENDGAME_LIMIT = 2500;

static const size_t PAWN_HASH_SIZE = 8192;

static const int SAFETY_TABLE[100] =  {
	0,   0,   1,   2,   3,   5,   7,   9,  12,  15,
	18,  22,  26,  30,  35,  39,  44,  50,  56,  62,
	68,  75,  82,  85,  89,  97, 105, 113, 122, 131,
	140, 150, 169, 180, 191, 202, 213, 225, 237, 248,
	260, 272, 283, 295, 307, 319, 330, 342, 354, 366,
	377, 389, 401, 412, 424, 436, 448, 459, 471, 483,
	494, 500, 500, 500, 500, 500, 500, 500, 500, 500,
	500, 500, 500, 500, 500, 500, 500, 500, 500, 500,
	500, 500, 500, 500, 500, 500, 500, 500, 500, 500,
	500, 500, 500, 500, 500, 500, 500, 500, 500, 500
};

static const int KNIGHT_MOBILITY[] = { // Number of squares attacked
	-30, -15, -5, 0, 5, 10, 15, 20, 25 // More squares, better mobility
};

static const int BISHOP_MOBILITY[] = {
	-30, -15, -5, 0, 5, 10, 15, 20, 25, 30, 35, 40, 45, 50
};

static const int ROOK_MOBILITY[] = {
	-30, -15, -5, 0, 5, 10, 15, 20, 25, 30, 35, 40, 45, 50, 50
};

static const int QUEEN_MOBILITY[] = {
	-30, -15, -5, 0, 5, 10, 15, 20, 25, 30, 35, 40, 45, 50, 
	50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50
};

struct PawnEntry {
	PawnEntry() : mKey(0), mStructure{} {}
	U64 getKey() const {
		return mKey;
	}
	const std::array<int, PLAYER_SIZE>& getStructure() const {
		return mStructure;
	}
	const std::array<int, PLAYER_SIZE>& getMaterial() const	{
		return mMaterial;
	}
	U64 mKey;
	std::array<int, PLAYER_SIZE> mStructure;
	std::array<int, PLAYER_SIZE> mMaterial;
};

static std::array<PawnEntry, PAWN_HASH_SIZE> pawnHash;

inline void init_eval() {
	std::fill(pawnHash.begin(), pawnHash.end(), PawnEntry());
}

inline PawnEntry* probe(U64 pKey) {
	return &pawnHash[pKey % pawnHash.size()];
}

inline void store(U64 pKey, const std::array<int, PLAYER_SIZE>& pStructure, const std::array<int, PLAYER_SIZE>& pMaterial) {
	pawnHash[pKey % pawnHash.size()].mKey = pKey;
	pawnHash[pKey % pawnHash.size()].mStructure = pStructure;
	pawnHash[pKey % pawnHash.size()].mMaterial = pMaterial;
}

class Evaluate {
public:
	Evaluate(const State& pState);
	//template<PieceType PT> int outpost(Square p, Color c);
	void evalPawns(const Color c);
	void evalPieces(const Color c);
	void evalAttacks(Color c);
	int getScore() const;
	friend std::ostream& operator<<(std::ostream& o, const Evaluate& e);
private:
	int mGamePhase;
	const State& mState;
	int mScore;
	std::array<int, PLAYER_SIZE> mMobility;
	std::array<int, PLAYER_SIZE> mKingSafety;
	std::array<int, PLAYER_SIZE> mPawnStructure;
	std::array<int, PLAYER_SIZE> mMaterial;
	std::array<int, PLAYER_SIZE> mAttacks;
	std::array<std::array<U64, PIECE_TYPES_SIZE>, PLAYER_SIZE> mPieceAttacksBB;
	std::array<U64, PLAYER_SIZE> mAllAttacksBB;
};

#endif

///