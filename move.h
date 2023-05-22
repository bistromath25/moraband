/**
 * Moraband, known in antiquity as Korriban, was an 
 * Outer Rim planet that was home to the ancient Sith 
 **/

#ifndef MOVE_H
#define MOVE_H

#include <algorithm>
#include <vector>
#include <array>
#include <iterator>

#include <string>

#include "defs.h"

typedef uint16_t Move;

struct MoveEntry {
	Move move;
	int score;
};

inline bool noScore(const MoveEntry & entry) {
	return entry.score == 0;
}

inline bool operator==(const MoveEntry& entry, const Move move) {
	return entry.move == move;
}

inline bool operator<(const MoveEntry& entry1, const MoveEntry& entry2) {
	return entry1.score < entry2.score;
}

static const Move NULL_MOVE = 0;
static const Move SRC_MASK = 0x003F;
static const Move DST_MASK = 0x0FC0;
static const Move PIECE_PROMOTION_MASK = 0x7000;
static const Move CASTLE_FLAG = 0x8000;

inline Move makeMove(Square src, Square dst) {
	return static_cast<Move>(src) | static_cast<Move>(dst) << 6;
}

inline Move makeMove(Square src, Square dst, PieceType promo) {
	return static_cast<Move>(src) | static_cast<Move>(dst) << 6 | static_cast<Move>(promo) << 12;
}

inline Move makeCastle(Square src, Square dst) {
	return static_cast<Move>(src) | static_cast<Move>(dst) << 6 | CASTLE_FLAG;
}

inline Square getSrc(Move m) {
	return static_cast<Square>(m & SRC_MASK);
}

inline Square getDst(Move m) {
	return static_cast<Square>((m & DST_MASK) >> 6);
}

inline PieceType getPiecePromotion(Move m) {
	return static_cast<PieceType>((m & PIECE_PROMOTION_MASK) >> 12);
}

inline bool isPromotion(Move m) {
	return getPiecePromotion(m);
}

inline bool isCastle(Move m) {
	return m & CASTLE_FLAG;
}

inline std::string to_string(Move m) {
	std::string res;
	Square src, dst;

	src = getSrc(m);
	dst = getDst(m);

	res = SQUARE_TO_STRING[src] + SQUARE_TO_STRING[dst];

	switch (getPiecePromotion(m)) {
		case PIECETYPE_KNIGHT:
			res += "n";
			break;
		case PIECETYPE_BISHOP:
			res += "b";
			break;
		case PIECETYPE_ROOK:
			res += "r";
			break;
		case PIECETYPE_QUEEN:
			res += "q";
			break;
		default:
			break;
	}

	return res;
}

#endif