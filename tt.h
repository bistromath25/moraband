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
	TTEntry() {};
	U64 getKey() const {
		return key ^ data;
	}
	Move getMove() const {
		return Move(data & MOVE_MASK);
	}
	void set(Move move, U64 flag, U64 depth, U64 score, U64 key) {
		data = move | (flag << FLAG_SHIFT) | (depth << DEPTH_SHIFT) | (score << SCORE_SHIFT);
		this->key = key ^ data;
	}
	int getFlag() const {
		return (data >> FLAG_SHIFT) & FLAG_MASK;
	}
	int getDepth() const {
		return (data >> DEPTH_SHIFT) & DEPTH_MASK;
	}
	int getScore() const {
		return int(data >> SCORE_SHIFT);
	}
	void clear() {
		key = data = 0;
	}
private:
	U64 key;
	U64 data;
};

struct TTCluster {
	TTEntry& getEntry(U64 key) {
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
	void clear() {
		for (TTEntry& entry : entries) {
			entry.clear();
		}
	}
private:
	TTEntry entries[CLUSTER_SIZE];
};

/* Transposition table to store search information */
struct TranspositionTable {
	TranspositionTable() {
		size = (1 << 20) / sizeof(TTCluster) * DEFAULT_HASH_SIZE;
		table = new TTCluster[size];
		clear();
	}
	~TranspositionTable() {
		delete[] table;
	}
	TranspositionTable(int mb) {
		resize(mb); // MB
	}
	void resize(int mb) {
		if (mb < 1) {
			mb = 1;
		}
		size = ((1 << 20) / sizeof(TTCluster)) * mb;
		delete[] table;
		table = new TTCluster[size];
		clear();
	}
	TTEntry probe(U64 key) const {
		int index = hash(key);
		return table[index].getEntry(key);
	}
	void insert(Move move, U64 flag, U64 depth, U64 score, U64 key) {
		int index = hash(key);
		table[index].getEntry(key).set(move, flag, depth, score, key);
	}
	void clear() {
		for (int i = 0; i < size; ++i) {
			table[i].clear();
		}
	}
	int hash(U64 key) const {
		return key % size;
	}
private:
	TTCluster* table;
	int size;
};

extern TranspositionTable tt;

#endif