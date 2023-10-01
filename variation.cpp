/**
 * Moraband, known in antiquity as Korriban, was an 
 * Outer Rim planet that was home to the ancient Sith 
 **/

#include "variation.h"
#include "eval.h"

Variation::Variation() : isMatingLine(false), sz(0) {}

void Variation::pushToPv(Move move, U64 key, int ply, int score) {
	// Check if this is a mating line.
	isMatingLine = std::abs(score) >= CHECKMATE_BOUND ? true : false;

	// Find the indicies using the triangular forumla.
	int copyTo = triangularIndex(ply);
	int copyFromStart = triangularIndex(ply + 1);
	int copyFromEnd = copyFromStart + MAX_PLY - ply - 1;

	// Store the current move.
	pv[copyTo++] = std::make_pair(move, key);

	// Copy from the previous iteration.
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
		// Push moves to the move list.
		MoveList moveList(c);

		// If the next pv move is in the move list, make the move.
		if (moveList.contains(nextMove)) {
			c.makeMove(nextMove);
		}
		// If the next pv move is not found, break the loop.
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