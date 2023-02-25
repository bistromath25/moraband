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

static const int KNIGHT_THREAT = 3; // Threats on enemy King
static const int BISHOP_THREAT = 3;
static const int ROOK_THREAT = 4;
static const int QUEEN_THREAT = 5;

static const int PAWN_WEIGHT_MG = 90;
static const int KNIGHT_WEIGHT_MG = 310;
static const int BISHOP_WEIGHT_MG = 351;
static const int ROOK_WEIGHT_MG = 480;
static const int QUEEN_WEIGHT_MG = 1139;

static const int PAWN_WEIGHT_EG = 94;
static const int KNIGHT_WEIGHT_EG = 281;
static const int BISHOP_WEIGHT_EG = 317;
static const int ROOK_WEIGHT_EG = 645;
static const int QUEEN_WEIGHT_EG = 1110;

static const int PAWN_WEIGHT = (PAWN_WEIGHT_MG + PAWN_WEIGHT_EG) / 2;
static const int KNIGHT_WEIGHT = (KNIGHT_WEIGHT_MG + KNIGHT_WEIGHT_EG) / 2;
static const int BISHOP_WEIGHT = (BISHOP_WEIGHT_MG + BISHOP_WEIGHT_EG) / 2;
static const int ROOK_WEIGHT = (ROOK_WEIGHT_MG + ROOK_WEIGHT_EG) / 2;
static const int QUEEN_WEIGHT = (QUEEN_WEIGHT_MG + QUEEN_WEIGHT_EG) / 2;
static const int KING_WEIGHT = 32767;

static const int PieceValue[] =  {
    PAWN_WEIGHT_MG,
    KNIGHT_WEIGHT_MG,
    BISHOP_WEIGHT_MG,
    ROOK_WEIGHT_MG,
    QUEEN_WEIGHT,
    KING_WEIGHT,
    0 // none
};

// Pawn eval
static const int PAWN_PASSED = 20;
static const int PAWN_PASSED_CANDIDATE = 10;
static const int PAWN_CONNECTED = 10;
static const int PAWN_ISOLATED = -10;
static const int PAWN_DOUBLED = -5;
static const int PAWN_FULL_BACKWARDS = -15;
static const int PAWN_BACKWARDS = -5;

static const int BAD_BISHOP = -20;
static const int TRAPPED_ROOK = -25;
static const int STRONG_PAWN_ATTACK = -80; // -80
static const int WEAK_PAWN_ATTACK = -40; // -40
static const int HANGING = -30;

// Assorted bonuses
static const int BISHOP_PAIR_MG = 55;
static const int BISHOP_PAIR_EG = 77;
static const int ROOK_OPEN_FILE = 33;
static const int ROOK_ON_SEVENTH_RANK = 44;
static const int KNIGHT_OUTPOST = 26;
static const int BISHOP_OUTPOST = 14;

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
	  0,   0,   0,   1,   1,   2,   3,   4,   5,   6,
      8,  10,  13,  16,  20,  25,  30,  36,  42,  48,
	 55,  62,  70,  80,  90, 100, 110, 120, 130, 140,
    150, 160, 170, 180, 190, 200, 210, 220, 230, 240,
    250, 260, 270, 280, 290, 300, 310, 320, 330, 340,
    350, 360, 370, 380, 390, 400, 410, 420, 430, 440,
    450, 460, 470, 480, 490, 500, 510, 520, 530, 540,
    550, 560, 570, 580, 590, 600, 610, 620, 630, 640,
    650, 650, 650, 650, 650, 650, 650, 650, 650, 650,
    650, 650, 650, 650, 650, 650, 650, 650, 650, 650
};

static const int KNIGHT_MOBILITY[2][9] = {
    { -50, -30, -10, 0, 10, 20, 25, 30, 35 },
    { -60, -30, -10, 0, 10, 20, 25, 30, 35 }
};

static const int BISHOP_MOBILITY[2][14] = {
	{ -30, -15, -5, 0, 5, 10, 15, 20, 25, 30, 35, 40, 45, 50 },
	{ -40, -15, -5, 0, 5, 10, 15, 20, 25, 30, 35, 40, 45, 50 }
};

static const int ROOK_MOBILITY[2][15] = {
	{ -30, -15, -5, 0, 5, 10, 15, 20, 25, 30, 35, 40, 45, 50, 50 },
	{ -40, -15, -5, 0, 10, 20, 30, 40, 50, 55, 60, 65, 70, 75, 80 }
};

static const int QUEEN_MOBILITY[2][28] = {
	{ -30, -20, -10, 0, 5, 10, 15, 20, 25, 30, 35, 40, 45, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50 },
    { -40, -20, -10, 0, 5, 10, 15, 20, 25, 30, 35, 40, 45, 50, 55, 60, 65, 70, 75, 80, 85, 90, 90, 90, 90, 90, 90, 90 }
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

struct PawnHashTable {
	PawnHashTable() {}
	void clear() {
		std::fill(table.begin(), table.end(), PawnEntry());
	}
	PawnEntry* probe(U64 pKey) {
		return &table[pKey % table.size()];
	}
	void store(U64 pKey, const std::array<int, PLAYER_SIZE>& pStructure, const std::array<int, PLAYER_SIZE>& pMaterial) {
		table[pKey % table.size()].mKey = pKey;
		table[pKey % table.size()].mStructure = pStructure;
		table[pKey % table.size()].mMaterial = pMaterial;
	}
	std::array<PawnEntry, PAWN_HASH_SIZE> table;
};

extern PawnHashTable ptable;

class Evaluate {
public:
	Evaluate(const State& pState);
	void evaluate();
	void evalPawns(const Color c);
	void evalPieces(const Color c);
	void evalAttacks(Color c);
	void evalOutpost(Square p, PieceType pt, Color c);
	int getScore() const;
	int getTaperedScore(int mg, int eg);
	friend std::ostream& operator<<(std::ostream& o, const Evaluate& e);
private:
	int mGamePhase;
	const State& mState;
	int mScore;
	std::array<int, PLAYER_SIZE> mMobility;
	std::array<int, PLAYER_SIZE> mKingSafety;
	std::array<int, PLAYER_SIZE> mPawnStructure;
	std::array<int, PLAYER_SIZE> mMaterial;
	//std::array<int, PLAYER_SIZE> mPst;
	std::array<int, PLAYER_SIZE> mAttacks;
	std::array<std::array<U64, PIECE_TYPES_SIZE>, PLAYER_SIZE> mPieceAttacksBB;
	std::array<U64, PLAYER_SIZE> mAllAttacksBB;
};

#endif

///