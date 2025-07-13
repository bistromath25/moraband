/**
 * Moraband, known in antiquity as Korriban, was an 
 * Outer Rim planet that was home to the ancient Sith 
 **/

#ifndef EVAL_H
#define EVAL_H

#include "position.h"
#include "pst.h"
#include <algorithm>
#include <array>
#include <iomanip>
#include <iostream>

struct Score {
    Score() : mg(0), eg(0) {}
    Score(int m, int e) : mg(m), eg(e) {}
    Score(const Score &s) : mg(s.mg), eg(s.eg) {}
    Score operator+(const int s) { return Score(mg + s, eg + s); }
    Score operator*(const int s) { return Score(mg * s, eg * s); }
    Score operator+(const Score &s) { return Score(mg + s.mg, eg + s.eg); }
    Score operator-(const Score &s) { return Score(mg - s.mg, eg - s.eg); }
    Score &operator+=(const int s) {
        mg += s;
        eg += s;
        return *this;
    }
    Score &operator-=(const int s) {
        mg -= s;
        eg -= s;
        return *this;
    }
    Score &operator+=(const Score &s) {
        mg += s.mg;
        eg += s.eg;
        return *this;
    }
    Score &operator-=(const Score &s) {
        mg -= s.mg;
        eg -= s.eg;
        return *this;
    }
    int score(int phase) const { return (mg * (256 - phase) + eg * phase) / 256; }
    int score() const { return mg; }

#ifndef TUNE
private:
#endif
    int mg;
    int eg;
};

extern Score TEMPO_BONUS; // Side-to-move bonus
extern int CONTEMPT;

extern int KNIGHT_THREAT; // Threats on enemy King
extern int BISHOP_THREAT;
extern int ROOK_THREAT;
extern int QUEEN_THREAT;

extern Score PAWN_WEIGHT;
extern Score KNIGHT_WEIGHT;
extern Score BISHOP_WEIGHT;
extern Score ROOK_WEIGHT;
extern Score QUEEN_WEIGHT;

extern const Score PIECE_VALUE[7];

extern Score KNIGHT_MOBILITY[9];
extern Score BISHOP_MOBILITY[14];
extern Score ROOK_MOBILITY[15];
extern Score QUEEN_MOBILITY[28];

// Pawn eval
enum PAWN_PASSED_TYPES {
    CANNOT_ADVANCE,
    UNSAFE_ADVANCE,
    PROTECTED_ADVANCE,
    SAFE_ADVANCE
};
extern Score PAWN_PASSED[4][7];
extern Score PAWN_PASSED_CANDIDATE;
extern Score PAWN_CONNECTED;
extern Score PAWN_ISOLATED;
extern Score PAWN_DOUBLED;
extern Score PAWN_FULL_BACKWARDS;
extern Score PAWN_BACKWARDS;
extern Score PAWN_SHIELD_CLOSE;
extern Score PAWN_SHIELD_FAR;
extern Score PAWN_SHIELD_MISSING;
extern Score STRONG_PAWN_ATTACK;
extern Score WEAK_PAWN_ATTACK;
extern Score HANGING;
extern Score KNIGHT_PAWN_PENALTY;
extern Score ROOK_PAWN_BONUS;

extern Score BISHOP_PAIR;
extern Score BAD_BISHOP;
extern Score ROOK_OPEN_FILE;
extern Score ROOK_ON_SEVENTH_RANK;
extern Score KNIGHT_OUTPOST;
extern Score BISHOP_OUTPOST;

const int CHECKMATE = 32767;
const int PSEUDO_CHECKMATE = 5000;
const int CHECKMATE_BOUND = CHECKMATE - MAX_PLY;
const int STALEMATE = 0;
const int DRAW = 0;

const int SAFETY_TABLE[100] = {
    0, 0, 0, 1, 1, 2, 3, 4, 5, 6,
    8, 10, 13, 16, 20, 25, 30, 36, 42, 48,
    55, 62, 70, 80, 90, 100, 110, 120, 130, 140,
    150, 160, 170, 180, 190, 200, 210, 220, 230, 240,
    250, 260, 270, 280, 290, 300, 310, 320, 330, 340,
    350, 360, 370, 380, 390, 400, 410, 420, 430, 440,
    450, 460, 470, 480, 490, 500, 510, 520, 530, 540,
    550, 560, 570, 580, 590, 600, 610, 620, 630, 640,
    650, 650, 650, 650, 650, 650, 650, 650, 650, 650,
    650, 650, 650, 650, 650, 650, 650, 650, 650, 650};

extern int KING_RING[2][64];
extern Score KING_RING_ATTACK[2][5];

const int PAWN_HASH_SIZE = 2;

/* Pawn hash table entry */
struct PawnEntry {
    PawnEntry() : key(0), structure{} {}
    U64 getKey() const {
        return key;
    }
    const std::array<Score, PLAYER_SIZE> &getStructure() const {
        return structure;
    }
    const std::array<Score, PLAYER_SIZE> &getMaterial() const {
        return material;
    }
    void clear() {
        key = 0;
        std::fill(structure.begin(), structure.end(), Score{});
        std::fill(material.begin(), material.end(), Score{});
    }
    U64 key;
    std::array<Score, PLAYER_SIZE> structure;
    std::array<Score, PLAYER_SIZE> material;
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
    PawnEntry *probe(U64 key) {
        return &table[key % size];
    }
    void store(U64 key, const std::array<Score, PLAYER_SIZE> &structure, const std::array<Score, PLAYER_SIZE> &material) {
        table[key % size].key = key;
        table[key % size].structure = structure;
        table[key % size].material = material;
    }
    PawnEntry *table;
    int size;
};

extern PawnHashTable ptable;

/* Evaluation and related functions */
class Evaluate {
public:
    Evaluate(const Position &s);
    void evalPawns(const Color c);
    void evalPieces(const Color c);
    void evalAttacks(const Color c);
    void evalOutposts(const Color c);
    void evalPawnShield(const Color c);
    void evalPawnShield(const Color c, U64 pawnShieldMask);
    int getScore() const;
    friend std::ostream &operator<<(std::ostream &o, const Evaluate &e);

private:
    int gamePhase;
    const Position &position;
    Score score;
    std::array<Score, PLAYER_SIZE> mobility;
    std::array<Score, PLAYER_SIZE> king_safety;
    std::array<Score, PLAYER_SIZE> pawn_structure;
    std::array<Score, PLAYER_SIZE> material;
    std::array<Score, PLAYER_SIZE> attacks;
    std::array<std::array<U64, PIECE_TYPES_SIZE>, PLAYER_SIZE> piece_attacks_bb;
    std::array<U64, PLAYER_SIZE> all_attacks_bb;
};

void initKingRing();

#endif
