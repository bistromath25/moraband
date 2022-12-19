#include "board.h"

U64 square_bb[BOARD_SIZE];
U64 file_bb[BOARD_SIZE];
U64 rank_bb[BOARD_SIZE];
U64 pawn_attacks[PLAYER_SIZE][BOARD_SIZE];
U64 pawn_push[PLAYER_SIZE][BOARD_SIZE];
U64 pawn_dbl_push[PLAYER_SIZE][BOARD_SIZE];
U64 between_dia[BOARD_SIZE][BOARD_SIZE];
U64 between_hor[BOARD_SIZE][BOARD_SIZE];
U64 between[BOARD_SIZE][BOARD_SIZE];
U64 coplanar[BOARD_SIZE][BOARD_SIZE];
U64 adj_files[BOARD_SIZE];
U64 in_front[PLAYER_SIZE][BOARD_SIZE];
U64 king_net_bb[PLAYER_SIZE][BOARD_SIZE];
U64 outpost_area[PLAYER_SIZE];

const U64 KNIGHT_MOVES[BOARD_SIZE] = {
	U64(0x0000000000020400), U64(0x0000000000050800), U64(0x00000000000A1100), U64(0x0000000000142200),
	U64(0x0000000000284400), U64(0x0000000000508800), U64(0x0000000000A01000), U64(0x0000000000402000),
	U64(0x0000000002040004), U64(0x0000000005080008), U64(0x000000000A110011), U64(0x0000000014220022),
	U64(0x0000000028440044), U64(0x0000000050880088), U64(0x00000000A0100010), U64(0x0000000040200020),
	U64(0x0000000204000402), U64(0x0000000508000805), U64(0x0000000A1100110A), U64(0x0000001422002214),
	U64(0x0000002844004428), U64(0x0000005088008850), U64(0x000000A0100010A0), U64(0x0000004020002040),
	U64(0x0000020400040200), U64(0x0000050800080500), U64(0x00000A1100110A00), U64(0x0000142200221400),
	U64(0x0000284400442800), U64(0x0000508800885000), U64(0x0000A0100010A000), U64(0x0000402000204000),
	U64(0x0002040004020000), U64(0x0005080008050000), U64(0x000A1100110A0000), U64(0x0014220022140000),
	U64(0x0028440044280000), U64(0x0050880088500000), U64(0x00A0100010A00000), U64(0x0040200020400000),
	U64(0x0204000402000000), U64(0x0508000805000000), U64(0x0A1100110A000000), U64(0x1422002214000000),
	U64(0x2844004428000000), U64(0x5088008850000000), U64(0xA0100010A0000000), U64(0x4020002040000000),
	U64(0x0400040200000000), U64(0x0800080500000000), U64(0x1100110A00000000), U64(0x2200221400000000),
	U64(0x4400442800000000), U64(0x8800885000000000), U64(0x100010A000000000), U64(0x2000204000000000),
	U64(0x0004020000000000), U64(0x0008050000000000), U64(0x00110A0000000000), U64(0x0022140000000000),
	U64(0x0044280000000000), U64(0x0088500000000000), U64(0x0010A00000000000), U64(0x0020400000000000)
};

const U64 KING_MOVES[BOARD_SIZE] = {
	U64(0x0000000000000302), U64(0x0000000000000705), U64(0x0000000000000E0A), U64(0x0000000000001C14),
	U64(0x0000000000003828), U64(0x0000000000007050), U64(0x000000000000E0A0), U64(0x000000000000C040),
	U64(0x0000000000030203), U64(0x0000000000070507), U64(0x00000000000E0A0E), U64(0x00000000001C141C),
	U64(0x0000000000382838), U64(0x0000000000705070), U64(0x0000000000E0A0E0), U64(0x0000000000C040C0),
	U64(0x0000000003020300), U64(0x0000000007050700), U64(0x000000000E0A0E00), U64(0x000000001C141C00),
	U64(0x0000000038283800), U64(0x0000000070507000), U64(0x00000000E0A0E000), U64(0x00000000C040C000),
	U64(0x0000000302030000), U64(0x0000000705070000), U64(0x0000000E0A0E0000), U64(0x0000001C141C0000),
	U64(0x0000003828380000), U64(0x0000007050700000), U64(0x000000E0A0E00000), U64(0x000000C040C00000),
	U64(0x0000030203000000), U64(0x0000070507000000), U64(0x00000E0A0E000000), U64(0x00001C141C000000),
	U64(0x0000382838000000), U64(0x0000705070000000), U64(0x0000E0A0E0000000), U64(0x0000C040C0000000),
	U64(0x0003020300000000), U64(0x0007050700000000), U64(0x000E0A0E00000000), U64(0x001C141C00000000),
	U64(0x0038283800000000), U64(0x0070507000000000), U64(0x00E0A0E000000000), U64(0x00C040C000000000),
	U64(0x0302030000000000), U64(0x0705070000000000), U64(0x0E0A0E0000000000), U64(0x1C141C0000000000),
	U64(0x3828380000000000), U64(0x7050700000000000), U64(0xE0A0E00000000000), U64(0xC040C00000000000),
	U64(0x0203000000000000), U64(0x0507000000000000), U64(0x0A0E000000000000), U64(0x141C000000000000),
	U64(0x2838000000000000), U64(0x5070000000000000), U64(0xA0E0000000000000), U64(0x40C0000000000000)
};

U64 bishopMoves[BOARD_SIZE];
U64 rookMoves[BOARD_SIZE];

void bb_init() {
	for (Square sq_src = first_sq; sq_src <= last_sq; ++sq_src) {
		U64 bit = 1ULL << sq_src;
		square_bb[sq_src] = bit;
		pawn_attacks[WHITE][sq_src] = ((bit & NOT_H_FILE) << 7) | ((bit & NOT_A_FILE) << 9);
		pawn_attacks[BLACK][sq_src] = ((bit & NOT_H_FILE) >> 9) | ((bit & NOT_A_FILE) >> 7);
		pawn_push[WHITE][sq_src] = bit << 8;
		pawn_push[BLACK][sq_src] = bit >> 8;

		U64 front = east_fill(bit) | west_fill(bit);
		in_front[WHITE][sq_src] = north_fill(front << 8);
		in_front[BLACK][sq_src] = south_fill(front >> 8);

		king_net_bb[WHITE][sq_src] = KING_MOVES[sq_src] | (KING_MOVES[sq_src] << 16);
		king_net_bb[BLACK][sq_src] = KING_MOVES[sq_src] | (KING_MOVES[sq_src] >> 16);

		file_bb[sq_src] = north_fill(bit) | south_fill(bit);
		rank_bb[sq_src] = east_fill(bit) | west_fill(bit);
		
		bishopMoves[sq_src] = (north_east_fill(bit) | north_west_fill(bit) | south_east_fill(bit) | south_west_fill(bit)) & ~bit;
		rookMoves[sq_src] = (north_fill(bit) | south_fill(bit) | east_fill(bit) | west_fill(bit)) & ~bit;

		if (bit & RANK_2) {
			pawn_dbl_push[WHITE][sq_src] = bit << 16;
			pawn_dbl_push[BLACK][sq_src] = 0;
		}
		else if (bit & RANK_7) {
			pawn_dbl_push[WHITE][sq_src] = 0;
			pawn_dbl_push[BLACK][sq_src] = bit >> 16;
		}
		else {
			pawn_dbl_push[WHITE][sq_src] = 0;
			pawn_dbl_push[BLACK][sq_src] = 0;
		}
		for (Square sq_dst = H1; sq_dst <= A8; ++sq_dst) {
			adj_files[sq_dst] = sq_dst & FILE_A ? FILE_B
				: sq_dst & FILE_B ? FILE_A | FILE_C
					: sq_dst & FILE_C ? FILE_B | FILE_D
						: sq_dst & FILE_D ? FILE_C | FILE_E
							: sq_dst & FILE_E ? FILE_D | FILE_F
								: sq_dst & FILE_F ? FILE_E | FILE_G
									: sq_dst & FILE_G ? FILE_F | FILE_H
										: FILE_G;

			U64 r_attacks, b_attacks, occ;
			occ = 1ULL << sq_src | 1ULL << sq_dst;

			r_attacks = Rmagic(sq_src, 0);
			b_attacks = Bmagic(sq_src, 0);

			r_attacks &= (r_attacks & occ ? Rmagic(sq_dst, 0) : 0);
			b_attacks &= (b_attacks & occ ? Bmagic(sq_dst, 0) : 0);

			coplanar[sq_src][sq_dst] = r_attacks | b_attacks;

			r_attacks = Rmagic(sq_src, occ);
			b_attacks = Bmagic(sq_src, occ);

			r_attacks &= (r_attacks & occ ? Rmagic(sq_dst, occ) : 0);
			b_attacks &= (b_attacks & occ ? Bmagic(sq_dst, occ) : 0);

			between_dia[sq_src][sq_dst] = b_attacks;
			between_hor[sq_src][sq_dst] = r_attacks;
			between[sq_src][sq_dst] = b_attacks | r_attacks;
		}
	}
	outpost_area[WHITE] = RANK_4 | RANK_5 | RANK_6 | RANK_7;
	outpost_area[BLACK] = RANK_2 | RANK_3 | RANK_4 | RANK_5;
}

void print_bb(U64 bb) {
	const U64 MSB = 0x8000000000000000ULL;
	std::string res;
	for (int i = 0; i < 64; ++i) {
		if (i % 8 == 0) {
			if (i != 0) {
				res.append("\n");
			}
			for (int j = 0; j < 8; ++j) {
				res.append("+-");
			}
			res.append("+\n|");
		}
		U64 bit = pow(2, i);
		if (bb << i & MSB) {
			res.append("1|");
		}
		else {
			res.append("0|");
		}
	}
	res.append("\n");
	for (int j = 0; j < 8; ++j) {
		res.append("+-");
	}
    res.append("+\n");
    std::cout << res;
}