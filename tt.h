#ifndef TT_H
#define TT_H

#include "board.h"
#include "move.h"
#include <cstring>
#include <vector>

const int DEFAULT_HASH_SIZE = 256;
const int MIN_HASH_SIZE = 1;
const int MAX_HASH_SIZE = 65536;

struct TableEntry {
	U64 key;
	Move best;
	NodeType type;
	int depth;  
	int score;
	bool ancient;
};

inline std::ostream& operator<<(std::ostream& os, const TableEntry& tableEntry) {
	os << "Best Move: " << to_string(tableEntry.best) << '\n'
		<< "    Depth: " << tableEntry.depth << '\n'
			<< "    Score: " << tableEntry.score << '\n'
				<< "     Node: " << (tableEntry.type == pv  ? "PV" 
					: tableEntry.type == cut ? "CUT" 
						: "ALL")
							<< '\n';
	return os;
}

class TranspositionTable {
public:
	TranspositionTable() {
		table.resize(DEFAULT_HASH_SIZE * 1024);
	}
	~TranspositionTable() {}
	const TableEntry* probe(U64 key) const {
		return table.data() + key % table.size();
	}
	TableEntry& operator[](U64 key) {
		return table[key % table.size()];
	}
	void clear() {
		for (TableEntry& tableEntry : table) {
			tableEntry.key = 0;
			tableEntry.best = NULL_MOVE;
			tableEntry.type = pv;
			tableEntry.depth = 0;
			tableEntry.score = 0;
			tableEntry.ancient = true;
		}
	}
	void setAncient() {
		for (TableEntry& tableEntry : table) {
			tableEntry.ancient = true;
		}
	}
	std::size_t size() const {
		return table.size();
	}
	void resize(int size_mb) {
		table.clear();
		table.resize(size_mb * 1000 * 1000 / sizeof(TableEntry));
		clear();
	}
	void store(U64 key, Move best, NodeType type, int depth, int score) {
		int i = key % table.size();
		if (table[i].depth <= depth || table[i].ancient || type == pv) {
			table[i].key = key;
			table[i].best = best;
			table[i].type = type;
			table[i].depth = depth;
			table[i].score = score;
			table[i].ancient = false;
		}
	}
private:
	std::vector<TableEntry> table;
};

extern TranspositionTable ttable;

#endif

///