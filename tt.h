/**
 * Moraband, known in antiquity as Korriban, was an 
 * Outer Rim planet that was home to the ancient Sith 
 **/

#ifndef TT_H
#define TT_H

#include "defs.h"
#include "move.h"

#include <cinttypes>
#include <memory>

const int DEFAULT_HASH_SIZE = 256;
const int MIN_HASH_SIZE = 1;
const int MAX_HASH_SIZE = 65536;
extern int HASH_SIZE;

const int FLAG_EXACT = 1;
const int FLAG_UPPER = 2;
const int FLAG_LOWER = 3;
const int FLAG_SHIFT = 21;
const int DEPTH_SHIFT = 23;
const int SCORE_SHIFT = 32;
const int MOVE_MASK = 0x1fffff;
const int FLAG_MASK = 0x3;
const int DEPTH_MASK = 0x7f;
const int CLUSTER_SIZE = 4;

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

/* Transposition table to store search information */
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
