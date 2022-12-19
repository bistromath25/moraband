/* move.h */

#ifndef MOVE_H
#define MOVE_H

#include <string>
#include "defs.h"

typedef unsigned int Move;

struct MoveEntry {
	Move move;
	int score;
};

bool no_score(MoveEntry &entry) {
	return entry.score == 0;
}

bool operator == (MoveEntry &entry, Move move) {
	return entry.move == move;
}

bool operator < (MoveEntry &entry1, MoveEntry &entry2) {
	return entry1.score < entry2.score;
}

const Move NULL_MOVE = 0;
const Move SRC_MASK = 0x003F;
const Move DEST_MASK = 0x0FC0;
const Move PIECE_PROMOTION_MASK = 0x7000;
const Move CASTLE_FLAG = 0x8000;

Move make_move(Square sq_src, Square sq_dest) {
	return static_cast<Move>(sq_src) | static_cast<Move>(sq_dest) << 6;
}

Move make_move(Square sq_src, Square sq_dest, PieceType pr) {
	return static_cast<Move>(sq_src) | static_cast<Move>(sq_dest) << 6 | static_cast<Move>(pr) << 12;
}

Move make_castling_move(Square sq_src, Square sq_dest) {
	return static_cast<Move>(sq_src) | static_cast<Move>(sq_dest) << 6 | CASTLE_FLAG;
}

Square get_src(Move move) {
	return static_cast<Square>(move & SRC_MASK);
}

Square get_dest(Move move) {
	return static_cast<Square>((move & DEST_MASK) >> 6);
}

PieceType get_piece_promotion(Move move) {
	return static_cast<PieceType>((move & PIECE_PROMOTION_MASK) >> 12);
}

bool is_promotion(Move move) {
	return get_piece_promotion(move);
}

bool is_castling(Move move) {
	return move & CASTLE_FLAG;
}

std::string to_string(Move move) {
	std::string res;
	Square src, dest;
	
	src = get_src(move);
	dest = get_dest(move);
	
	res = SQ[src] + SQ[dest];
	
	auto piece = get_piece_promotion(move);
	if (piece == knight) res += "n";
	else if (piece == bishop) res += "b";
	else if (piece == rook) res += "r";
	else if (piece == queen) res += "q";
	
	return res; 
}

#endif