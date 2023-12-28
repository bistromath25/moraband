/**
 * Moraband, known in antiquity as Korriban, was an 
 * Outer Rim planet that was home to the ancient Sith 
 **/

#include "variation.h"
#include "eval.h"

Variation::Variation() : isMatingLine(false), sz(0) {}

int Variation::size() const {
	return sz;
}

void Variation::pushToPv(Move move, U64 key, int ply, int score) {
	isMatingLine = std::abs(score) >= CHECKMATE_BOUND ? true : false;
	int copyTo = getIndex(ply);
	int copyFromStart = getIndex(ply + 1);
	int copyFromEnd = copyFromStart + MAX_PLY - ply - 1;
	pv[copyTo++] = std::make_pair(move, key);
	std::copy(std::make_move_iterator(pv.begin() + copyFromStart), 
	std::make_move_iterator(pv.begin() + copyFromEnd), pv.begin() + copyTo);
}

U64 Variation::getPvKey(int ply) const {
	return pv[ply].second;
}

Move Variation::getPvMove(int ply) const {
	return pv[ply].first;
}

bool Variation::isMate() const {
	return isMatingLine;
}

int Variation::getMateInN() const {
	return sz / 2.0 + 0.5;
}

void Variation::clearPv() {
	sz = 0;
	std::fill(pv.begin(), pv.begin() + MAX_PLY, std::make_pair(0, 0));
}

void Variation::checkPv(Position& s) {
	Position c;
	Move nextMove;
	std::memmove(&c, &s, sizeof(s));
	for (int i = 0; i < MAX_PLY; ++i) {
		nextMove = pv[i].first;
		if (nextMove == NULL_MOVE) {
			sz = i;
			break;
		}
		MoveList moveList(c);
		if (moveList.contains(nextMove)) {
			c.makeMove(nextMove);
		}
		else {
			sz = i;
			break;
		}
	}
}

void Variation::printPv() {
	std::cout << " pv ";
	for (auto it = pv.begin(); it != pv.begin() + sz; ++it) {
		std::cout << to_string(it->first) << " ";
	}
}

int Variation::getIndex(int ply) {
	return ply * (2 * MAX_PLY + 1 - ply) / 2;
}