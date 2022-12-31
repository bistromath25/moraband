#ifndef PV_H
#define PV_H

#include <array>
#include <algorithm>
#include <iterator>
#include "state.h"
#include "movegen.h"
#include "defs.h"
#include "move.h"

static const int MAX_PV_SIZE = ((MAX_PLY * MAX_PLY) + MAX_PLY) / 2;

inline int triangularIndex(int ply) {
	return ply * (2 * MAX_PLY + 1 - ply) / 2;
}

class Variation {
public:
	Variation();
	void pushToPv(Move pMove, U64 pKey, int pPly, int pScore);
	U64 getPvKey(int pPly=0) const;
	Move getPvMove(int pPly=0) const;
	bool isMate() const;
	int getMateInN() const;
	void clearPv();
	void checkPv(State& pState);
	void printPv();
private:
	std::array<std::pair<Move, U64>, MAX_PV_SIZE> mPv; // (move, key)
	bool mMatingLine;
	std::size_t mSize;
};

#endif

///