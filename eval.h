/**
 * Moraband, known in antiquity as Korriban, was an 
 * Outer Rim planet that was home to the ancient Sith 
 **/

#pragma once

#ifndef EVAL_H
#define EVAL_H

#include <iostream>
#include <iomanip>
#include <array>
#include <algorithm>
#include "Position.h"
#include "pst.h"

extern int TEMPO_BONUS; // Side-to-move bonus
extern int CONTEMPT;

extern int KNIGHT_THREAT; // Threats on enemy King
extern int BISHOP_THREAT;
extern int ROOK_THREAT;
extern int QUEEN_THREAT;

extern int PAWN_WEIGHT_MG;
extern int KNIGHT_WEIGHT_MG;
extern int BISHOP_WEIGHT_MG;
extern int ROOK_WEIGHT_MG;
extern int QUEEN_WEIGHT_MG;

extern int PAWN_WEIGHT_EG;
extern int KNIGHT_WEIGHT_EG;
extern int BISHOP_WEIGHT_EG;
extern int ROOK_WEIGHT_EG;
extern int QUEEN_WEIGHT_EG;

extern const int KING_WEIGHT;
extern const int PieceValue[7];

extern int KNIGHT_MOBILITY[2][9];
extern int BISHOP_MOBILITY[2][14];
extern int ROOK_MOBILITY[2][15];
extern int QUEEN_MOBILITY[2][28];

// Pawn eval
enum PAWN_PASSED_TYPES {
	CANNOT_ADVANCE,
	UNSAFE_ADVANCE,
	PROTECTED_ADVANCE,
	SAFE_ADVANCE
};
extern int PAWN_PASSED[4][2][7];
extern int PAWN_PASSED_CANDIDATE;
extern int PAWN_CONNECTED;
extern int PAWN_ISOLATED;
extern int PAWN_DOUBLED;
extern int PAWN_FULL_BACKWARDS;
extern int PAWN_BACKWARDS;
extern int PAWN_SHIELD_CLOSE;
extern int PAWN_SHIELD_FAR;
extern int PAWN_SHIELD_MISSING;

extern int BAD_BISHOP;
extern int TRAPPED_ROOK;
extern int STRONG_PAWN_ATTACK;
extern int WEAK_PAWN_ATTACK;
extern int HANGING;

// Assorted bonuses
extern int BISHOP_PAIR_MG;
extern int BISHOP_PAIR_EG;
extern int ROOK_OPEN_FILE;
extern int ROOK_ON_SEVENTH_RANK;
extern int KNIGHT_OUTPOST;
extern int BISHOP_OUTPOST;

const int CHECKMATE = 32767;
const int CHECKMATE_BOUND = CHECKMATE - MAX_PLY;
const int STALEMATE = 0;
const int DRAW = 0;

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

const int PAWN_HASH_SIZE = 2;

/* Pawn hash table entry */
struct PawnEntry {
	PawnEntry() : key(0), structure{} {}
	U64 getKey() const {
		return key;
	}
	const std::array<int, PLAYER_SIZE>& getStructure() const {
		return structure;
	}
	const std::array<int, PLAYER_SIZE>& getMaterial() const	{
		return material;
	}
	void clear() {
		key = 0;
		std::fill(structure.begin(), structure.end(), 0);
		std::fill(material.begin(), material.end(), 0);
	}
	U64 key;
	std::array<int, PLAYER_SIZE> structure;
	std::array<int, PLAYER_SIZE> material;
};

/* Pawn hash table for pawn evaluation */
struct PawnHashTable {
	PawnHashTable() {
		size = (1 << 20) / sizeof(PawnEntry) * PAWN_HASH_SIZE;
		table = new PawnEntry[size];
		clear();
	}
	~PawnHashTable() {
		delete[] table;
	}
	void clear() {
		for (int i = 0; i < PAWN_HASH_SIZE; ++i) {
			table[i].clear();
		}
	}
	PawnEntry* probe(U64 key) {
		return &table[key % size];
	}
	void store(U64 key, const std::array<int, PLAYER_SIZE>& structure, const std::array<int, PLAYER_SIZE>& material) {
		table[key % size].key = key;
		table[key % size].structure = structure;
		table[key % size].material = material;
	}
	PawnEntry* table;
	int size;
};

extern PawnHashTable ptable;

/* Evaluation and related functions */
class Evaluate {
public:
	Evaluate(const Position& s);
	void evalPawns(const Color c);
	void evalPieces(const Color c);
	void evalAttacks(const Color c);
	void evalOutposts(const Color c);
	void evalPawnShield(const Color c);
	void evalPawnShield(const Color c, U64 pawnShieldMask);
	int getScore() const;
	int getTaperedScore(int mg, int eg);
	friend std::ostream& operator<<(std::ostream& o, const Evaluate& e);
private:
	int mGamePhase;
	const Position& position;
	int mScore;
	std::array<int, PLAYER_SIZE> mobility;
	std::array<int, PLAYER_SIZE> king_safety;
	std::array<int, PLAYER_SIZE> pawn_structure;
	std::array<int, PLAYER_SIZE> material;
	//std::array<int, PLAYER_SIZE> mPst;
	std::array<int, PLAYER_SIZE> attacks;
	std::array<std::array<U64, PIECE_TYPES_SIZE>, PLAYER_SIZE> piece_attacks_bb;
	std::array<U64, PLAYER_SIZE> all_attacks_bb;
};

#endif