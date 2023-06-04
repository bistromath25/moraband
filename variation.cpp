/**
 * Moraband, known in antiquity as Korriban, was an 
 * Outer Rim planet that was home to the ancient Sith 
 **/

#include "variation.h"
#include "eval.h"
//#include "io.h"

Variation::Variation() : mMatingLine(false), mSize(0) {}

void Variation::pushToPv(Move move, U64 key, int ply, int score) {
	// Check if this is a mating line.
	mMatingLine = std::abs(score) >= CHECKMATE_BOUND ? true : false;

	// Find the indicies using the triangular forumla.
	int copyTo = triangularIndex(ply);
	int copyFromStart = triangularIndex(ply + 1);
	int copyFromEnd = copyFromStart + MAX_PLY - ply - 1;

	// Store the current move.
	mPv[copyTo++] = std::make_pair(move, key);

	// Copy from the previous iteration.
	std::copy(std::make_move_iterator(mPv.begin() + copyFromStart), 
	std::make_move_iterator(mPv.begin() + copyFromEnd), mPv.begin() + copyTo);
}

U64 Variation::getPvKey(int ply) const {
	return mPv[ply].second;
}

Move Variation::getPvMove(int ply) const {
	return mPv[ply].first;
}

bool Variation::isMate() const {
	return mMatingLine;
}

int Variation::getMateInN() const {
	return mSize / 2.0 + 0.5;
}

void Variation::clearPv() {
	mSize = 0;
	std::fill(mPv.begin(), mPv.begin() + MAX_PLY, std::make_pair(0, 0));
}

void Variation::checkPv(State& s) {
	State c;
	Move nextMove;
	std::memmove(&c, &s, sizeof(s));
	for (int i = 0; i < MAX_PLY; ++i) {
		nextMove = mPv[i].first;
		if (nextMove == NULL_MOVE) {
			mSize = i;
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
			mSize = i;
			break;
		}
	}
}

void Variation::printPv() {
	std::cout << " pv ";
	for (auto it = mPv.begin(); it != mPv.begin() + mSize; ++it) {
		std::cout << to_string(it->first) << " ";
	}
}