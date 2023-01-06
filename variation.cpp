#include "variation.h"
#include "eval.h"
//#include "io.h"

Variation::Variation() : mMatingLine(false), mSize(0) {}

void Variation::pushToPv(Move pMove, U64 pKey, int pPly, int pScore) {
	// Check if this is a mating line
	mMatingLine = std::abs(pScore) + MAX_PLY >= CHECKMATE ? true : false;

	// Find the indicies using the triangular forumla
	int copyTo = triangularIndex(pPly);
	int copyFromStart = triangularIndex(pPly + 1);
	int copyFromEnd = copyFromStart + MAX_PLY - pPly - 1;

	// Store the current move
	mPv[copyTo++] = std::make_pair(pMove, pKey);

	// Copy from the previous iteration
	std::copy(std::make_move_iterator(mPv.begin() + copyFromStart), 
	std::make_move_iterator(mPv.begin() + copyFromEnd), mPv.begin() + copyTo);
}

U64 Variation::getPvKey(int pPly) const {
	return mPv[pPly].second;
}

Move Variation::getPvMove(int pPly) const {
	return mPv[pPly].first;
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

void Variation::checkPv(State& pState) {
	State c;
	Move nextMove;
	std::memmove(&c, &pState, sizeof(pState));
	for (int i = 0; i < MAX_PLY; ++i) {
		nextMove = mPv[i].first;
		if (nextMove == NULL_MOVE) {
			mSize = i;
			break;
		}
		// Push moves to the move list
		MoveList moveList(c);

		// If the next pv move is in the move list, make the move
		if (moveList.contains(nextMove)) {
			c.makeMove(nextMove);
		}
		// If the next pv move is not found, break the loop
		else {
			mSize = i;
			break;
		}
	}
}

void Variation::printPv() {
	std::cout << " pv ";
	//engine_log << " pv ";
	for (auto it = mPv.begin(); it != mPv.begin() + mSize; ++it) {
		std::cout << to_string(it->first) << " ";
		//engine_log << toString(it->first) << " ";
	}
}