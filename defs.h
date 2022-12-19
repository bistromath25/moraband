/* defs.h */

#ifndef DEFS_H
#define DEFS_H

#include <string>
#include <assert.h>
#include <sys/time.h>

typedef unsigned int U32;
typedef unsigned long long U64;

const int BOARD_SIZE = 64;
const int PLAYER_SIZE = 2;
const int PIECE_TYPES_SIZE = 6;
const int PIECE_MAX = 10;

const int CASTLING_RIGHTS[BOARD_SIZE] = {
	14, 15, 15, 12, 15, 15, 15, 13,
	15, 15, 15, 15, 15, 15, 15, 15,
	15, 15, 15, 15, 15, 15, 15, 15,
	15, 15, 15, 15, 15, 15, 15, 15,
	15, 15, 15, 15, 15, 15, 15, 15,
	15, 15, 15, 15, 15, 15, 15, 15,
	15, 15, 15, 15, 15, 15, 15, 15,
	11, 15, 15,  3, 15, 15, 15,  7
};

const int GAME_STAGE_SIZE = 2;

enum GameStage {
	middlegame, endgame
};

enum Color {
	white, black
};

enum PieceType {
	pawn,
	knight,
	bishop,
	rook,
	queen,
	king,
	none
};

const int PAWN_WEIGHT = 100;
const int KNIGHT_WEIGHT = 300;
const int BISHOP_WEIGHT = 300;
const int ROOK_WEIGHT = 500;
const int QUEEN_WEIGHT = 900;
const int KING_WEIGHT = 32767;

const int PIECE_VALUE[] = {
	PAWN_WEIGHT,
	KNIGHT_WEIGHT,
	BISHOP_WEIGHT,
	ROOK_WEIGHT,
	QUEEN_WEIGHT,
	KING_WEIGHT,
	0;
}

enum Square : U32 {
	H1, G1, F1, E1, D1, C1, B1, A1,
	H2, G2, F2, E2, D2, C2, B2, A2,
	H3, G3, F3, E3, D3, C3, B3, A3,
	H4, G4, F4, E4, D4, C4, B4, A4,
	H5, G5, F5, E5, D5, C5, B5, A5,
	H6, G6, F6, E6, D6, C6, B6, A6,
	H7, G7, F7, E7, D7, C7, B7, A7,
	H8, G8, F8, E8, D8, C8, B8, A8,
	no_sq = 64, first_sq = 0, last_sq = 63
};

const std::string SquareString[64] = {
	"h1", "g1", "f1", "e1", "d1", "c1", "b1", "a1",
	"h2", "g2", "f2", "e2", "d2", "c2", "b2", "a2",
	"h3", "g3", "f3", "e3", "d3", "c3", "b3", "a3",
	"h4", "g4", "f4", "e4", "d4", "c4", "b4", "a4",
	"h5", "g5", "f5", "e5", "d5", "c5", "b5", "a5",
	"h6", "g6", "f6", "e6", "d6", "c6", "b6", "a6",
	"h7", "g7", "f7", "e7", "d7", "c7", "b7", "a7",
	"h8", "g8", "f8", "e8", "d8", "c8", "b8", "a8"
};

enum File {
	file_a,
	file_b,
	file_c,
	file_d,
	file_e,
	file_f,
	file_g,
	file_h,
};

File file(Square sq) {
	return File(sq & 0x7);
}

enum Rank {
	rank_1,
	rank_2,
	rank_3,
	rank_4,
	rank_5,
	rank_6,
	rank_7,
	rank_8
};

Rank rank(Square sq) {
	return Rank(sq >> 3);
}

enum CastlingRights {
	white_castle_short = 1,
	white_castle_long = 2,
	black_castle_short = 4,
	black_castle_long = 8
};

enum Dir {
	north = 8,
	south = -8,
	east = -1,
	west = 1,
	north_east = 7,
	north_west = 9,
	south_east = -9,
	south_west = -7
};

// 




#define DEFS_H