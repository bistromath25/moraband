/**
 * Moraband, known in antiquity as Korriban, was an 
 * Outer Rim planet that was home to the ancient Sith 
 **/

#include "tt.h"

TTEntry::TTEntry() {}

U64 TTEntry::getKey() const {
	return key ^ data;
}

Move TTEntry::getMove() const {
	return Move(data & MOVE_MASK);
}

void TTEntry::set(Move move, U64 flag, U64 depth, U64 score, U64 key) {
	data = move | (flag << FLAG_SHIFT) | (depth << DEPTH_SHIFT) | (score << SCORE_SHIFT);
	this->key = key ^ data;
}

int TTEntry::getFlag() const {
    return (data >> FLAG_SHIFT) & FLAG_MASK;
}
int TTEntry::getDepth() const {
    return (data >> DEPTH_SHIFT) & DEPTH_MASK;
}
int TTEntry::getScore() const {
    return int(data >> SCORE_SHIFT);
}
void TTEntry::clear() {
    key = data = 0;
}

TTEntry& TTCluster::getEntry(U64 key) {
    for (TTEntry& entry : entries) { // Return anything that matches
        if (entry.getKey() == key) {
            return entry;
        }
    }
    int min_depth_index = 0;
    for (int i = 1; i < CLUSTER_SIZE; i++) { // Return minimum depth
        if (entries[i].getDepth() < entries[min_depth_index].getDepth()) {
            min_depth_index = i;
        }
    }
    return entries[min_depth_index];
}

void TTCluster::clear() {
    for (TTEntry& entry : entries) {
        entry.clear();
    }
}

/* Transposition table to store search information */
TranspositionTable::TranspositionTable() {
    size = (1 << 20) / sizeof(TTCluster) * DEFAULT_HASH_SIZE;
    table = new TTCluster[size];
    clear();
}

TranspositionTable::TranspositionTable(int mb) {
	resize(mb);
}

TranspositionTable::~TranspositionTable() {
	delete[] table;
}

void TranspositionTable::resize(int mb) {
    if (mb < 1) {
        mb = 1;
    }
    if (mb > MAX_HASH_SIZE) {
        mb = MAX_HASH_SIZE;
    }
    size = ((1 << 20) / sizeof(TTCluster)) * mb;
    delete[] table;
    table = new TTCluster[size];
    clear();
}

TTEntry TranspositionTable::probe(U64 key) const {
    int index = hash(key);
    return table[index].getEntry(key);
}
void TranspositionTable::insert(Move move, U64 flag, U64 depth, U64 score, U64 key) {
    int index = hash(key);
    table[index].getEntry(key).set(move, flag, depth, score, key);
}
void TranspositionTable::clear() {
    for (int i = 0; i < size; ++i) {
        table[i].clear();
    }
}
int TranspositionTable::hash(U64 key) const {
    return key % size;
}

TranspositionTable tt;