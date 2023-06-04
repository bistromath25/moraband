/**
 * Moraband, known in antiquity as Korriban, was an 
 * Outer Rim planet that was home to the ancient Sith 
 **/

#ifndef EVAL_H
#define EVAL_H

#include <iostream>
#include <iomanip>
#include <array>
#include <algorithm>
#include "state.h"
#include "pst.h"

const int TEMPO_BONUS = 8; // Side-to-move bonus
extern int CONTEMPT;

const int KNIGHT_THREAT = 3; // Threats on enemy King
const int BISHOP_THREAT = 3;
const int ROOK_THREAT = 4;
const int QUEEN_THREAT = 5;

const int PAWN_WEIGHT_MG = 90;
const int KNIGHT_WEIGHT_MG = 310;
const int BISHOP_WEIGHT_MG = 351;
const int ROOK_WEIGHT_MG = 480;
const int QUEEN_WEIGHT_MG = 1139;

const int PAWN_WEIGHT_EG = 94;
const int KNIGHT_WEIGHT_EG = 281;
const int BISHOP_WEIGHT_EG = 317;
const int ROOK_WEIGHT_EG = 645;
const int QUEEN_WEIGHT_EG = 1110;

const int KING_WEIGHT = 32767;

const int PieceValue[7] =  {
    PAWN_WEIGHT_MG,
    KNIGHT_WEIGHT_MG,
    BISHOP_WEIGHT_MG,
    ROOK_WEIGHT_MG,
    QUEEN_WEIGHT_MG,
    KING_WEIGHT,
    0 // none
};

// Pawn eval
enum PAWN_PASSED_TYPES {
	CANNOT_ADVANCE,
	UNSAFE_ADVANCE,
	PROTECTED_ADVANCE,
	SAFE_ADVANCE
};
const int PAWN_PASSED = 20;
const int PAWN_PASSED_CANDIDATE = 10;
const int PAWN_CONNECTED = 10;
const int PAWN_ISOLATED = -10;
const int PAWN_DOUBLED = -5;
const int PAWN_FULL_BACKWARDS = -15;
const int PAWN_BACKWARDS = -5;

const int PAWN_PASSED_ADVANCE[4][2][7] = {
	{ // Cannot advance
		{ 0, 4, 8, 12, 20, 60, 80 }, // Middlegame
		{ 0, 4, 8, 18, 25, 70, 100 } // Endgame
	},
	{ // Unsafe advance
		{ 0, 5, 10, 15, 25, 70, 100 },
		{ 0, 5, 10, 20, 28, 85, 150 }
	},
	{ // Protected advance
		{ 0, 7, 12, 17, 30, 80, 110 },
		{ 0, 7, 12, 22, 45, 120, 220 }
	},
	{ // Safe advance
		{ 0, 7, 15, 20, 40, 100, 150 },
		{ 0, 7, 15, 25, 50, 200, 300 }
	}
};

const int BAD_BISHOP = -20;
const int TRAPPED_ROOK = -25;
const int STRONG_PAWN_ATTACK = -80;
const int WEAK_PAWN_ATTACK = -40;
const int HANGING = -30;

// Assorted bonuses
const int BISHOP_PAIR_MG = 55;
const int BISHOP_PAIR_EG = 77;
const int ROOK_OPEN_FILE = 33;
const int ROOK_ON_SEVENTH_RANK = 44;
const int KNIGHT_OUTPOST = 26;
const int BISHOP_OUTPOST = 14;

const int CHECKMATE = 32767;
const int CHECKMATE_BOUND = CHECKMATE - MAX_PLY;
const int STALEMATE = 0;
const int DRAW = 0;

const size_t PAWN_HASH_SIZE = 8192;

const int SAFETY_TABLE[100] =  {
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

const int KNIGHT_MOBILITY[2][9] = {
    { -50, -30, -10, 0, 10, 20, 25, 30, 35 },
    { -60, -30, -10, 0, 10, 20, 25, 30, 35 }
};

const int BISHOP_MOBILITY[2][14] = {
	{ -30, -15, -5, 0, 5, 10, 15, 20, 25, 30, 35, 40, 45, 50 },
	{ -40, -15, -5, 0, 5, 10, 15, 20, 25, 30, 35, 40, 45, 50 }
};

const int ROOK_MOBILITY[2][15] = {
	{ -30, -15, -5, 0, 5, 10, 15, 20, 25, 30, 35, 40, 45, 50, 50 },
	{ -40, -15, -5, 0, 10, 20, 30, 40, 50, 55, 60, 65, 70, 75, 80 }
};

const int QUEEN_MOBILITY[2][28] = {
	{ -30, -20, -10, 0, 5, 10, 15, 20, 25, 30, 35, 40, 45, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50 },
    { -40, -20, -10, 0, 5, 10, 15, 20, 25, 30, 35, 40, 45, 50, 55, 60, 65, 70, 75, 80, 85, 90, 90, 90, 90, 90, 90, 90 }
};

/* Pawn hash table entry */
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

/* Pawn hash table for pawn evaluation */
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

/* Evaluation and related functions */
class Evaluate {
public:
	Evaluate(const State& s);
	void evalPawns(const Color c);
	void evalPieces(const Color c);
	void evalAttacks(const Color c);
	void evalOutposts(const Color c);
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