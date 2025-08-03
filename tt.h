/**
 * Moraband, known in antiquity as Korriban, was an 
 * Outer Rim planet that was home to the ancient Sith 
 **/

#ifndef TT_H
#define TT_H

#include "move.h"

constexpr int DEFAULT_HASH_SIZE = 256;
constexpr int MIN_HASH_SIZE = 1;
constexpr int MAX_HASH_SIZE = 65536;
extern int HASH_SIZE;

constexpr int FLAG_EXACT = 1;
constexpr int FLAG_UPPER = 2;
constexpr int FLAG_LOWER = 3;
constexpr int FLAG_SHIFT = 21;
constexpr int DEPTH_SHIFT = 23;
constexpr int SCORE_SHIFT = 32;
constexpr int MOVE_MASK = 0x1fffff;
constexpr int FLAG_MASK = 0x3;
constexpr int DEPTH_MASK = 0x7f;
constexpr int CLUSTER_SIZE = 4;

struct TTEntry {
    TTEntry();
    U64 getKey() const;
    Move getMove() const;
    void set(Move move, U64 flag, U64 depth, U64 score, U64 key);
    int getFlag() const;
    int getDepth() const;
    int getScore() const;
    void clear();

private:
    U64 key;
    U64 data;
};

struct alignas(32) TTCluster {
    TTEntry &getEntry(U64 key);
    void clear();

private:
    TTEntry entries[CLUSTER_SIZE];
};

/** Transposition table to store search information */
struct TranspositionTable {
    TranspositionTable();
    TranspositionTable(int mb);
    ~TranspositionTable();
    void resize(int mb);
    TTEntry probe(U64 key) const;
    void insert(Move move, U64 flag, U64 depth, U64 score, U64 key);
    void clear();
    int hash(U64 key) const;

private:
    TTCluster *table;
    int size;
};

extern TranspositionTable tt;

#endif
