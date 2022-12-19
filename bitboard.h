/* bitboard.h */

#ifndef BITBOARD_H
#define BITBOARD_H

#include <string>
#include <iostream>
#include "defs.h"

extern U64 square_bitboard[BOARD_SIZE];
extern U64 file_bitboard[BOARD_SIZE];
extern U64 rank_bitboard[BOARD_SIZE];
extern U64 pawn_attacks[PLAYER_SIZE][BOARD_SIZE];
extern U64 pawn_push[PLAYER_SIZE][BOARD_SIZE];

extern const U64 KNIGHT_MOVES[BOARD_SIZE];
extern const U64 KING_MOVES[BOARD_SIZE];
extern bishop_moves[BOARD_SIZE];
extern rook_moves[BOARD_SIZE];

const U64 DARK_SQUARES = 0xAA55AA55AA55AA55ULL;
const U64 LIGHT_SQUARES = 0x55AA55AA55AA55AAULL;

const U64 RANK_1 = 0x00000000000000FFULL;
const U64 RANK_2 = 0x000000000000FF00ULL;
const U64 RANK_3 = 0x0000000000FF0000ULL;
const U64 RANK_4 = 0x00000000FF000000ULL;
const U64 RANK_5 = 0x000000FF00000000ULL;
const U64 RANK_6 = 0x0000FF0000000000ULL;
const U64 RANK_7 = 0x00FF000000000000ULL;
const U64 RANK_8 = 0xFF00000000000000ULL;
const U64 RANK_PROMOTION = RANK_1 | RANK_8;
const U64 RANK_PAWN_START = RANK_2 | RANK_7;

const U64 FILE_A = 0x8080808080808080ULL;
const U64 FILE_B = 0x4040404040404040ULL;
const U64 FILE_C = 0x2020202020202020ULL;
const U64 FILE_D = 0x1010101010101010ULL;
const U64 FILE_E = 0x0808080808080808ULL;
const U64 FILE_F = 0x0404040404040404ULL;
const U64 FILE_G = 0x0202020202020202ULL;
const U64 FILE_H = 0x0101010101010101ULL;

void initialize_biboard();

U64 operator & (Square sq, U64 u) { return square_bitboard[sq] & u; }
U64 operator | (Square sq, U64 u) { return square_bitboard[sq] & u; }
U64 operator ^ (Square sq, U64 u) { return square_bitboard[sq] & u; }

int pop_count(U64 bb) {
	#if defined(_MSC_VER) && defined(_WIN64)
	return _mm_popcnt_u64(bb);
	#elif defined(__GNUC__)
	return __builtin_popcountll(bb);
	#else
	static const U64 m1 = 0x5555555555555555ull;
	static const U64 m2 = 0x3333333333333333ull;
	static const U64 m4 = 0x0f0f0f0f0f0f0f0full;
	static const U64 h1 = 0x0101010101010101ull;
	bb -= (bb >> 1) & m1;
	bb = (bb & m2) + ((bb >> 2) & m2);
	bb = (bb + (bb >> 4)) & m4;
	return static_cast<int>((bb * h1) >> 56);
	#endif
}

Square get_lsb(U64 bb) {
	assert(bb);
	#if defined(_MSC_VER)
		unsigned long index;
		#if defined(_M_AMD64) || defined(__WIN64)
			_BitScanForward64(&index, bb);
			return static_cast<Square>(index);
		#else
			if (_BitScanForward(&index, bb)) {
				return static_cast<Square>(index);
			}
			_BitScanForward(&index, bb >> 32);
			return static_cast<Square>(index);
		#endif
	#elif defined(__GNUC__)
		return static_cast<Square>(__builtin_ctzll(bb));
	#else
		static const U64 DeBrujin = 0x03f79d71b4cb0a89;
		static const int DeBrujin_table[64] = {
			0,  1, 48,  2, 57, 49, 28,  3,
			61, 58, 50, 42, 38, 29, 17,  4,
			62, 55, 59, 36, 53, 51, 43, 22,
			45, 39, 33, 30, 24, 18, 12,  5,
			63, 47, 56, 27, 60, 41, 37, 16,
			54, 35, 52, 21, 44, 32, 23, 11,
			46, 26, 40, 15, 34, 20, 31, 10,
			25, 14, 19,  9, 13,  8,  7,  6
		};
		return static_cast<Square>(DeBrujin_table[((bb & -bb) * DeBrujin) >> 58]);
	#endif
}

U64 get_lst_bitboard(U64 bb) {
	// assert(bb);
	return 1ULL << get_lsb(bb);
}

Square pop_lsb(U64 &bb) {
	U64 t = bb;
	bb &= (bb - 1);
	return get_lst(t);
}

U64 pop_lsb_bitboard(U64 &bb) {
	U64 t = bb;
	bb &= (bb - 1);
	return t ^ bb;
}

File get_file(U64 bb) {
	assert(bb);
	return File(get_lsb(bb) % 8);
}

Rank get_rank(U64 bb) {
	return Rank(get_lsb(bb) / 8);
}

U64 shift_up(U64 bb, Dir d) {
	return d == north ? bb << 8
		: d == north_east ? (bb & NOT_H_FILE) << 9
			: d == north_west ? (bb & NOT_A_FILE) << 7
				: 0;
}

U64 squares_of_color(Square sq) {
	return sq & LIGHT_SQUARES ? LIGHT_SQUARES : DARK_SQUARES;
}

U64 shift(U64 bb, Dir d) {
	return d == north ? bb << 8 : bb >> 8;
}

U64 shift_e(U64 bb, Dir d) {
	return d == north_east ? (bb & NOT_H_FILE) << 7 : (bb & NOT_H_FILE) >> 9;
}

U64 shift_w(U64 bb, Dir d) {
	return d == north_west ? (bb & NOT_A_FILE) << 9 : (bb & NOT_A_FILE) >> 7; 
}

U64 shift_ep(U64 bb, Dir d) {
	return d == east ? (bb & NOT_H_FILE) >> 1 : (bb & NOT_A_FILE) << 1;
}

void clear_bit(U64 &bb, int dest) {
	bb &= ~(square_bitboard[dest]);
}

void set_bit(U64 &bb, int dest) {
	bb |= square_bitboard[dest];
}

void move_bit(U64 &bb, int src, int dest) {
	bb ^= square_bitboard[src] | square_bitboard[dest];
}

void print_bitboard(U64);

#endif


