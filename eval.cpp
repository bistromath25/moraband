/**
 * Moraband, known in antiquity as Korriban, was an 
 * Outer Rim planet that was home to the ancient Sith 
 **/

#include "eval.h"

#define S(mg, eg) Score(mg, eg)

Score TEMPO_BONUS = S(16, 16); // Side-to-move bonus

int KNIGHT_THREAT = 3; // Threats on enemy King
int BISHOP_THREAT = 3;
int ROOK_THREAT = 4;
int QUEEN_THREAT = 5;

Score PAWN_WEIGHT = S(100, 104);
Score KNIGHT_WEIGHT = S(337, 281);
Score BISHOP_WEIGHT = S(365, 297);
Score ROOK_WEIGHT = S(477, 512);
Score QUEEN_WEIGHT = S(1025, 936);

const Score KING_WEIGHT = S(32767, 32767);

const Score PIECE_VALUE[7] =  {
	PAWN_WEIGHT,
	KNIGHT_WEIGHT,
	BISHOP_WEIGHT,
	ROOK_WEIGHT,
	QUEEN_WEIGHT,
	KING_WEIGHT,
	S(0, 0) // none
};

Score KNIGHT_MOBILITY[9] = {
	S(-50, -60), S(-30, -30), S(-10, -10), S(0, 0), S(10, 10), S(20, 20), S(25, 25), S(30, 30), S(35, 35)
};

Score BISHOP_MOBILITY[14] = {
	S(-30, -40), S(-15, -15), S(-5, -5), S(0, 0), S(5, 5), S(10, 10), S(15, 15), S(20, 20), S(25, 25), S(30, 30), S(35, 35), S(40, 40), S(45, 45), S(50, 50)
};

Score ROOK_MOBILITY[15] = {
	S(-30, -40), S(-15, -15), S(-5, -5), S(0, 0), S(5, 10), S(10, 20), S(15, 30), S(20, 40), S(25, 50), S(30, 55), S(35, 60), S(40, 65), S(45, 70), S(50, 75), S(50, 80)	
};

Score QUEEN_MOBILITY[28] = {
	S(-30, -40), S(-20, -20), S(-10, -10), S(0, 0), S(5, 5), S(10, 10), S(15, 15), S(20, 20), S(25, 25), S(30, 30), S(35, 35), S(40, 40), S(45, 45), S(50, 50), S(50, 55), S(50, 60), S(50, 65), S(50, 70), S(50, 75), S(50, 80), S(50, 85), S(50, 90), S(50, 90), S(50, 90), S(50, 90), S(50, 90), S(50, 90), S(50, 90)
};

// Passed pawn eval
Score PAWN_PASSED[4][7] = {
	{ // Cannot advance
		S(0, 0), S(4, 4), S(8, 8), S(15, 20), S(30, 35), S(60, 65), S(70, 80)
	},
	{ // Unsafe advance
		S(0, 0), S(5, 5), S(46, 46), S(53, 55), S(62, 70), S(64, 75), S(89, 99)
	},
	{ // Protected advance
		S(0, 0), S(7, 7), S(12, 12), S(17, 22), S(30, 45), S(80, 100), S(115, 200)
	},
	{ // Safe advance
		S(0, 0), S(8, 8), S(15, 15), S(20, 24), S(40, 60), S(100, 190), S(150, 290)
	}
};
Score PAWN_PASSED_CANDIDATE = S(20, 40);
Score PAWN_CONNECTED = S(10, 15);
Score PAWN_ISOLATED = S(-10, -5);
Score PAWN_DOUBLED = S(-10, -5);
Score PAWN_FULL_BACKWARDS = S(-15, -7);
Score PAWN_BACKWARDS = S(-10, -5);
Score PAWN_SHIELD_CLOSE = S(10, 0);
Score PAWN_SHIELD_FAR = S(8, 0);
Score PAWN_SHIELD_MISSING = S(-8, 0);
Score STRONG_PAWN_ATTACK = S(-60, -60);
Score WEAK_PAWN_ATTACK = S(-30, -30);
Score HANGING = S(-15, -15);
Score KNIGHT_PAWN_PENALTY = S(-1, -1);
Score ROOK_PAWN_BONUS = S(3, 1);

Score BISHOP_PAIR = S(50, 80);
Score BAD_BISHOP = S(-10, -20);
Score TRAPPED_ROOK = S(-25, -50);
Score ROOK_OPEN_FILE = S(30, 15);
Score ROOK_ON_SEVENTH_RANK = S(40, 20);
Score KNIGHT_OUTPOST = S(45, 10);
Score BISHOP_OUTPOST = S(25, 5);

int KING_RING[2][64];
Score KING_RING_ATTACK[2][5] = {
	{
		S(40, -15), S(30, -10), S(45, -5), S(40, -10), S(35, 5)
	},
	{
		S(25, -8), S(25, -1), S(25, -3), S(20, -3), S(20, 8)
	}
};

PawnHashTable ptable;

/* Evaluation and related functions */
Evaluate::Evaluate(const Position& s) : position(s), material{}, pawn_structure{}, mobility{}, king_safety{}, attacks{}, piece_attacks_bb{}, all_attacks_bb{} {
	// Tapered-evaluation with 256 phases, 0 (Opening/Middlegame) and 255 (Endgame)
	gamePhase = position.getGamePhase();
	Color c = position.getOurColor();
	// Check for entry in pawn hash table
	if (position.getPieceCount<PIECETYPE_PAWN>()) {
		const PawnEntry* pawnEntry = ptable.probe(position.getPawnKey());
		if (pawnEntry && pawnEntry->getKey() == position.getPawnKey()) {
			pawn_structure = pawnEntry->getStructure();
			material = pawnEntry->getMaterial();
		}
		else {
			evalPawns(WHITE);
			evalPawns(BLACK);
			ptable.store(position.getPawnKey(), pawn_structure, material);
		}
	}

	// Pawn attacks
	piece_attacks_bb[WHITE][PIECETYPE_PAWN] |= (position.getPieceBB<PIECETYPE_PAWN>(WHITE) & NOT_A_FILE) << 9 & position.getOccupancyBB();
	piece_attacks_bb[WHITE][PIECETYPE_PAWN] |= (position.getPieceBB<PIECETYPE_PAWN>(WHITE) & NOT_H_FILE) << 7 & position.getOccupancyBB();
	piece_attacks_bb[BLACK][PIECETYPE_PAWN] |= (position.getPieceBB<PIECETYPE_PAWN>(BLACK) & NOT_A_FILE) >> 7 & position.getOccupancyBB();
	piece_attacks_bb[BLACK][PIECETYPE_PAWN] |= (position.getPieceBB<PIECETYPE_PAWN>(BLACK) & NOT_H_FILE) >> 9 & position.getOccupancyBB();
	all_attacks_bb[WHITE] |= piece_attacks_bb[WHITE][PIECETYPE_PAWN];
	all_attacks_bb[BLACK] |= piece_attacks_bb[BLACK][PIECETYPE_PAWN];

	evalPieces(WHITE);
	evalPieces(BLACK);
	evalPawnShield(WHITE);
	evalPawnShield(BLACK);
	evalAttacks(WHITE);
	evalAttacks(BLACK);
	score = mobility[c] - mobility[!c] + king_safety[c] - king_safety[!c] + pawn_structure[c] - pawn_structure[!c] + material[c] - material[!c] + attacks[c] - attacks[!c];
	score += S(position.getPstScore(MIDDLEGAME), position.getPstScore(ENDGAME));
	score += TEMPO_BONUS;
}

int Evaluate::getScore() const {
	//return score + CONTEMPT;
	return score.score(gamePhase) + CONTEMPT;
}

/*
int Evaluate::getTaperedScore(int mg, int eg) {
	return (mg * (256 - gamePhase) + eg * gamePhase) / 256;
}
*/

void Evaluate::evalOutposts(const Color c) {
	for (Square p : position.getPieceList<PIECETYPE_KNIGHT>(c)) {
		if (p == no_sq) {
			break;
		}
		if (!(p & outpost_area[c]) || !(pawn_attacks[!c][p] & position.getPieceBB<PIECETYPE_PAWN>(c)) || in_front[c][p] & adj_files[p] & position.getPieceBB<PIECETYPE_PAWN>(!c)) {
			continue;
		}
		material[c] += KNIGHT_OUTPOST;
		if (!position.getPieceBB<PIECETYPE_KNIGHT>(!c) && !position.getPieceBB<PIECETYPE_BISHOP>(!c) & squares_of_color(p)) {
			material[c] += KNIGHT_OUTPOST;
		}
	}
	for (Square p : position.getPieceList<PIECETYPE_BISHOP>(c)) {
		if (p == no_sq) {
			break;
		}
		if (!(p & outpost_area[c]) || !(pawn_attacks[!c][p] & position.getPieceBB<PIECETYPE_PAWN>(c)) || in_front[c][p] & adj_files[p] & position.getPieceBB<PIECETYPE_PAWN>(!c)) {
			continue;
		}
		material[c] += BISHOP_OUTPOST;
		if (!position.getPieceBB<PIECETYPE_KNIGHT>(!c) && !position.getPieceBB<PIECETYPE_BISHOP>(!c) & squares_of_color(p)) {
			material[c] += BISHOP_OUTPOST;
		}
	}
}

/* Evaluate pawn structure */
void Evaluate::evalPawns(const Color c) {
	const int dir = c == WHITE ? 8 : -8;

	for (Square p : position.getPieceList<PIECETYPE_PAWN>(c)) {
		if (p == no_sq) {
			break;
		}
		int r = c == WHITE ? rank(p) : 7 - rank(p);

		material[c] += PAWN_WEIGHT;

		if (!((file_bb[p] | adj_files[p]) & in_front[c][p] & position.getPieceBB<PIECETYPE_PAWN>(!c))) {
			if ((p + dir) & position.getOccupancyBB()) {
				pawn_structure[c] += PAWN_PASSED[CANNOT_ADVANCE][r];
			}
			else {
				if (!position.attacked(p + dir)) {
					pawn_structure[c] += PAWN_PASSED[SAFE_ADVANCE][r];
				}
				else {
					if (position.defended(p + dir, c)) {
						pawn_structure[c] += PAWN_PASSED[PROTECTED_ADVANCE][r];
					}
					else {
						pawn_structure[c] += PAWN_PASSED[UNSAFE_ADVANCE][r];
					}
				}
			}
		}
		else if (!(file_bb[p] & in_front[c][p] & position.getPieceBB<PIECETYPE_PAWN>(!c))) {
			int sentries, helpers;
			assert(p < 56 && p > 7);

			sentries = pop_count(pawn_attacks[c][p + dir] & position.getPieceBB<PIECETYPE_PAWN>(!c));
			if (sentries > 0) {
				helpers = pop_count(pawn_attacks[!c][p + dir] & position.getPieceBB<PIECETYPE_PAWN>(c));
				if (helpers >= sentries) {
					pawn_structure[c] += PAWN_PASSED_CANDIDATE;
				}
				else if (!(~in_front[c][p] & adj_files[p] & position.getPieceBB<PIECETYPE_PAWN>(c))) {
					if (pawn_attacks[c][p] & position.getPieceBB<PIECETYPE_PAWN>(c)) {
						if (sentries == 2) {
							pawn_structure[c] += PAWN_FULL_BACKWARDS;
						}
						else {
							pawn_structure[c] += PAWN_BACKWARDS;
						}
					}
				}
			}
		}

		if (!(adj_files[p] & position.getPieceBB<PIECETYPE_PAWN>(c))) {
			pawn_structure[c] += PAWN_ISOLATED;
		}
		else if (rank_bb[p - dir] & position.getPieceBB<PIECETYPE_PAWN>(c) & adj_files[p]) {
			pawn_structure[c] += PAWN_CONNECTED;
		}
		
		if (pop_count(file_bb[p] & position.getPieceBB<PIECETYPE_PAWN>(c)) > 1) {
			pawn_structure[c] += PAWN_DOUBLED;
		}
	}
}

void Evaluate::evalPieces(const Color c) {
	U64 moves, pins;
	U64 mobilityNet;
	int king_threats = 0;
	U64 bottomRank = c == WHITE ? RANK_1 : RANK_8;
	Square kingSq = position.getKingSquare(c);
	const int missingPawns = 8 - position.getPieceCount<PIECETYPE_PAWN>(c);

	mobilityNet = position.getEmptyBB(); // All squares not attacked by pawns
	if (c == WHITE) {
		mobilityNet &= ~((position.getPieceBB<PIECETYPE_PAWN>(BLACK) & NOT_A_FILE) >> 7 | (position.getPieceBB<PIECETYPE_PAWN>(BLACK) & NOT_H_FILE) >> 9);
	}
	else {
		mobilityNet &= ~((position.getPieceBB<PIECETYPE_PAWN>(WHITE) & NOT_A_FILE) << 9 | (position.getPieceBB<PIECETYPE_PAWN>(WHITE) & NOT_H_FILE) << 7);
	}
	
	pins = position.getPinsBB(c);

	for (Square p : position.getPieceList<PIECETYPE_KNIGHT>(c)) {
		if (p == no_sq) {
			break;
		}
		material[c] += KNIGHT_WEIGHT;
		if (square_bb[p] & pins) {
			mobility[c] += KNIGHT_MOBILITY[0];
			continue;
		}
		moves = position.getAttackBB<PIECETYPE_KNIGHT>(p);
		piece_attacks_bb[c][PIECETYPE_KNIGHT] |= moves & position.getOccupancyBB();
		all_attacks_bb[c] |= piece_attacks_bb[c][PIECETYPE_KNIGHT];
		mobility[c] += KNIGHT_MOBILITY[pop_count(moves & mobilityNet)];
		material[c] += KNIGHT_PAWN_PENALTY * missingPawns;
		if (moves & king_net_bb[!c][position.getKingSquare(!c)] & mobilityNet) {
			king_threats += KNIGHT_THREAT;
		}
	}

	for (Square p : position.getPieceList<PIECETYPE_BISHOP>(c)) {
		if (p == no_sq) {
			break;
		}
		material[c] += BISHOP_WEIGHT;
		moves = position.getAttackBB<PIECETYPE_BISHOP>(p);
		if (square_bb[p] & pins) {
			moves &= coplanar[p][kingSq];
		}
		piece_attacks_bb[c][PIECETYPE_BISHOP] = moves & position.getOccupancyBB();
		all_attacks_bb[c] |= piece_attacks_bb[c][PIECETYPE_BISHOP];
		mobility[c] += BISHOP_MOBILITY[pop_count(moves & mobilityNet)];
		material[c] += BAD_BISHOP * pop_count(squares_of_color(p) & position.getPieceBB<PIECETYPE_PAWN>(c));
		if (moves & king_net_bb[!c][position.getKingSquare(!c)] & mobilityNet) {
			king_threats += BISHOP_THREAT;
		}
	}
	
	for (Square p : position.getPieceList<PIECETYPE_ROOK>(c)) {
		if (p == no_sq) {
			break;
		}
		material[c] += ROOK_WEIGHT;
		moves = position.getAttackBB<PIECETYPE_ROOK>(p);
		if (square_bb[p] & pins) {
			moves &= coplanar[p][kingSq];
		}
		piece_attacks_bb[c][PIECETYPE_ROOK] = moves & position.getOccupancyBB();
		all_attacks_bb[c] |= piece_attacks_bb[c][PIECETYPE_ROOK];
		mobility[c] += ROOK_MOBILITY[pop_count(moves & mobilityNet)];
		material[c] += ROOK_PAWN_BONUS * missingPawns;
		if (moves & king_net_bb[!c][position.getKingSquare(!c)] & mobilityNet) {
			king_threats += ROOK_THREAT;
		}
	}
	
	for (Square p : position.getPieceList<PIECETYPE_QUEEN>(c)) {
		if (p == no_sq) {
			break;
		}
		material[c] += QUEEN_WEIGHT;
		moves = position.getAttackBB<PIECETYPE_QUEEN>(p);
		if (square_bb[p] & pins) {
			moves &= coplanar[p][kingSq];
		}
		piece_attacks_bb[c][PIECETYPE_QUEEN] = moves & position.getOccupancyBB();
		all_attacks_bb[c] |= piece_attacks_bb[c][PIECETYPE_QUEEN];
		mobility[c] += QUEEN_MOBILITY[pop_count(moves & mobilityNet)];
		if (moves & king_net_bb[!c][position.getKingSquare(!c)] & mobilityNet) {
			king_threats += QUEEN_THREAT;
		}
	}
	
	// Bishop pair
	if (position.getPieceCount<PIECETYPE_BISHOP>(c) >= 2) {
		material[c] += BISHOP_PAIR;
	}

	// Threats on enemy King
	king_safety[!c] -= SAFETY_TABLE[king_threats];

	// King ring attacks
	U64 kingRing1 = KING_RING[0][position.getKingSquare(!c)];
	U64 kingRing2 = KING_RING[1][position.getKingSquare(!c)];
	for (int pt = PIECETYPE_PAWN; pt <= PIECETYPE_QUEEN; ++pt) {
		king_safety[!c] -= KING_RING_ATTACK[0][pt] * pop_count(kingRing1 & piece_attacks_bb[c][pt]);
		king_safety[!c] -= KING_RING_ATTACK[1][pt] * pop_count(kingRing2 & piece_attacks_bb[c][pt]);
	}
}

void Evaluate::evalPawnShield(const Color c) {
	Square kingSq = position.getKingSquare(c);
	int r = c == WHITE ? rank(kingSq) : 7 - rank(kingSq);
	if (r <= 1) {
		if (c == WHITE) {
			if (file(kingSq) <= 2) { // Kingside
				evalPawnShield(WHITE, square_bb[F2] | square_bb[G2] | square_bb[H2]);
			}
			else if (file(kingSq) >= 5) { // Queenside
				evalPawnShield(WHITE, square_bb[A2] | square_bb[B2] | square_bb[C2]);
			}
		}
		else {
			if (file(kingSq) <= 2) { // Kingside
				evalPawnShield(BLACK, square_bb[F7] | square_bb[G7] | square_bb[H7]);
			}
			else if (file(kingSq) >= 5) { // Queenside
				evalPawnShield(BLACK, square_bb[A7] | square_bb[B7] | square_bb[C7]);
			}
		}
	}
}

void Evaluate::evalPawnShield(const Color c, U64 pawnShieldCloseMask) {
	U64 pawnShieldFarMask = shift(pawnShieldCloseMask, c == WHITE ? N : S);
	U64 pawnsBB = position.getPieceBB<PIECETYPE_PAWN>(c);

	U64 pawnShieldClose = pawnsBB & pawnShieldCloseMask;
	U64 pawnShieldFar = pawnsBB & pawnShieldFarMask;
	pawnShieldFar &= ~shift(pawnShieldClose, c == WHITE ? N : S);

	U64 pawnShieldMissing = ~(pawnShieldClose | shift(pawnShieldFar, c == WHITE ? S : N)) & pawnShieldCloseMask;

	king_safety[c] += PAWN_SHIELD_CLOSE * pop_count(pawnShieldClose);
	king_safety[c] += PAWN_SHIELD_FAR * pop_count(pawnShieldFar);
	king_safety[c] += PAWN_SHIELD_MISSING * pop_count(pawnShieldMissing);

	U64 themPawnsBB = position.getPieceBB<PIECETYPE_PAWN>(!c);
	while (pawnShieldMissing) {
		if (themPawnsBB & file(pop_lsb(pawnShieldMissing))) {
			king_safety[c] += PAWN_SHIELD_MISSING;
		}
	}
}

void Evaluate::evalAttacks(const Color c) {
	U64 attackedByPawn, hanging;
	attackedByPawn = piece_attacks_bb[!c][PIECETYPE_PAWN] & (position.getOccupancyBB(c) ^ position.getPieceBB<PIECETYPE_PAWN>(c));

	while (attackedByPawn) {
		Square to = pop_lsb(attackedByPawn);
		if (position.getAttackBB<PIECETYPE_PAWN>(to, c) & position.getPieceBB<PIECETYPE_PAWN>(!c) & all_attacks_bb[!c]) {
			attacks[c] += STRONG_PAWN_ATTACK;
		}
		else {
			attacks[c] += WEAK_PAWN_ATTACK;
		}
	}

	hanging = all_attacks_bb[!c] & position.getOccupancyBB(c) & ~all_attacks_bb[c];
	
	attacks[c] += HANGING * pop_count(hanging);
}

std::ostream& operator<<(std::ostream& os, const Evaluate& e) {
	Color c = e.position.getOurColor();
	std::string us = c == WHITE ? "White" : "Black";
	std::string them = c == WHITE ? "Black" : "White";
	int pstMid = e.position.getPstScore(MIDDLEGAME) * (256 - e.gamePhase) / 256;
	int pstLate = e.position.getPstScore(ENDGAME) * e.gamePhase / 256;

	os  << "-------------------------------------------------------------\n"
		<< "| Evaluation Type |    " << us << "    |    " << them 
		<< "    |    Total    |\n"
		<< "-------------------------------------------------------------\n"
		<< "| Material        |"
		<< std::setw(12) << e.material[c].score() << " |"
		<< std::setw(12) << e.material[!c].score() << " |"
		<< std::setw(12) << e.material[c].score() - e.material[!c].score() << " |\n"
		<< "-------------------------------------------------------------\n"
		<< "| Mobility        |" 
		<< std::setw(12) << e.mobility[c].score() << " |" 
		<< std::setw(12) << e.mobility[!c].score() << " |"
		<< std::setw(12) << e.mobility[c].score() - e.mobility[!c].score() << " |\n"
		<< "-------------------------------------------------------------\n"
		<< "| Pawn Structure  |"
		<< std::setw(12) << e.pawn_structure[c].score() << " |" 
		<< std::setw(12) << e.pawn_structure[!c].score() << " |"
		<< std::setw(12) << e.pawn_structure[c].score() - e.pawn_structure[!c].score() 
		<< " |\n"
		<< "-------------------------------------------------------------\n"
		<< "| King Safety     |"
		<< std::setw(12) << e.king_safety[c].score() << " |" 
		<< std::setw(12) << e.king_safety[!c].score() << " |"
		<< std::setw(12) << e.king_safety[c].score() - e.king_safety[!c].score() << " |\n"
		<< "-------------------------------------------------------------\n"
		<< "| Attacks         |"
		<< std::setw(12) << e.attacks[c].score() << " |" 
		<< std::setw(12) << e.attacks[!c].score() << " |"
		<< std::setw(12) << e.attacks[c].score() - e.attacks[!c].score() << " |\n"
		<< "-------------------------------------------------------------\n"
		<< "| PST             |"
		<< std::setw(12) << pstMid << " |" 
		<< std::setw(12) << pstLate << " |"
		<< std::setw(12) << pstMid + pstLate << " |\n"
		<< "-------------------------------------------------------------\n"
		<< "| Total           |             |             |"
		<< std::setw(12) << e.getScore() << " |\n"
		<< "-------------------------------------------------------------\n";

	return os;
}

void initKingRing() {
	for (int ring = 1; ring <= 2; ++ring) {
		for (int i = 0; i < 8; ++i) {
			for (int j = 0; j < 8; ++j) {
				KING_RING[ring - 1][i * 8 + j] = 0ULL;
				for (int y = i - ring; y <= i + ring; ++y) {
					for (int x = j - ring; x <= j + ring; ++x) {
						if (y < 0 || y >= 8 || x < 0 || x >= 8) {
							continue;
						}
						else if (y == i - ring || y == i + ring || x == j - ring || x == j + ring) {
							KING_RING[ring - 1][i * 8 + j] |= 1ULL << (y * 8 + x);
						}
					}
				}
			}
		}
	}
}