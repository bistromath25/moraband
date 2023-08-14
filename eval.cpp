/**
 * Moraband, known in antiquity as Korriban, was an 
 * Outer Rim planet that was home to the ancient Sith 
 **/

#include "eval.h"

int TEMPO_BONUS = 16; // Side-to-move bonus

int KNIGHT_THREAT = 3; // Threats on enemy King
int BISHOP_THREAT = 3;
int ROOK_THREAT = 4;
int QUEEN_THREAT = 5;

int PAWN_WEIGHT_MG = 100;
int KNIGHT_WEIGHT_MG = 337;
int BISHOP_WEIGHT_MG = 365;
int ROOK_WEIGHT_MG = 477;
int QUEEN_WEIGHT_MG = 1025;

int PAWN_WEIGHT_EG = 104;
int KNIGHT_WEIGHT_EG = 281;
int BISHOP_WEIGHT_EG = 297;
int ROOK_WEIGHT_EG = 512;
int QUEEN_WEIGHT_EG = 936;

const int KING_WEIGHT = 32767;

const int PieceValue[7] =  {
    PAWN_WEIGHT_MG,
    KNIGHT_WEIGHT_MG,
    BISHOP_WEIGHT_MG,
    ROOK_WEIGHT_MG,
    QUEEN_WEIGHT_MG,
    KING_WEIGHT,
    0 // none
};

int KNIGHT_MOBILITY[2][9] = {
    { -50, -30, -10, 0, 10, 20, 25, 30, 35 },
    { -60, -30, -10, 0, 10, 20, 25, 30, 35 }
};

int BISHOP_MOBILITY[2][14] = {
	{ -30, -15, -5, 0, 5, 10, 15, 20, 25, 30, 35, 40, 45, 50 },
	{ -40, -15, -5, 0, 5, 10, 15, 20, 25, 30, 35, 40, 45, 50 }
};

int ROOK_MOBILITY[2][15] = {
	{ -30, -15, -5, 0, 5, 10, 15, 20, 25, 30, 35, 40, 45, 50, 50 },
	{ -40, -15, -5, 0, 10, 20, 30, 40, 50, 55, 60, 65, 70, 75, 80 }
};

int QUEEN_MOBILITY[2][28] = {
	{ -30, -20, -10, 0, 5, 10, 15, 20, 25, 30, 35, 40, 45, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50 },
    { -40, -20, -10, 0, 5, 10, 15, 20, 25, 30, 35, 40, 45, 50, 55, 60, 65, 70, 75, 80, 85, 90, 90, 90, 90, 90, 90, 90 }
};

// Pawn eval
int PAWN_PASSED[4][2][7] = {
	{ // Cannot advance
		{ 0, 4, 8, 15, 30, 60, 70 }, // Middlegame
		{ 0, 4, 8, 20, 35, 65, 80 } // Endgame
	},
	{ // Unsafe advance
		{ 0, 5, 46, 53, 62, 64, 89 },
		{ 0, 5, 46, 55, 70, 75, 99 }
	},
	{ // Protected advance
		{ 0, 7, 12, 17, 30, 80, 115 },
		{ 0, 7, 12, 22, 45, 100, 200 }
	},
	{ // Safe advance
		{ 0, 8, 15, 20, 40, 100, 150 },
		{ 0, 8, 15, 24, 60, 190, 290 }
	}
};
int PAWN_PASSED_CANDIDATE = 20;
int PAWN_CONNECTED = 10;
int PAWN_ISOLATED = -10;
int PAWN_DOUBLED = -10;
int PAWN_FULL_BACKWARDS = -15;
int PAWN_BACKWARDS = -10;
int PAWN_SHIELD_CLOSE = 10;
int PAWN_SHIELD_FAR = 8;
int PAWN_SHIELD_MISSING = -8;

int BAD_BISHOP = -10;
int TRAPPED_ROOK = -25;
int STRONG_PAWN_ATTACK = -60;
int WEAK_PAWN_ATTACK = -30;
int HANGING = -15;

// Assorted bonuses
int BISHOP_PAIR_MG = 50;
int BISHOP_PAIR_EG = 80;
int ROOK_OPEN_FILE = 75;
int ROOK_ON_SEVENTH_RANK = 60;
int KNIGHT_OUTPOST = 45;
int BISHOP_OUTPOST = 25;

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
	score += getTaperedScore(position.getPstScore(MIDDLEGAME), position.getPstScore(ENDGAME));
	score += TEMPO_BONUS;
}

int Evaluate::getScore() const {
	return score + CONTEMPT;
}

int Evaluate::getTaperedScore(int mg, int eg) {
	return (mg * (256 - gamePhase) + eg * gamePhase) / 256;
}

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
		int r = c == WHITE ? rank(p) : 8 - rank(p) - 1;

		material[c] += getTaperedScore(PAWN_WEIGHT_MG, PAWN_WEIGHT_EG);

		if (!((file_bb[p] | adj_files[p]) & in_front[c][p] & position.getPieceBB<PIECETYPE_PAWN>(!c))) {
			if ((p + dir) & position.getOccupancyBB()) {
				pawn_structure[c] += getTaperedScore(PAWN_PASSED[CANNOT_ADVANCE][MIDDLEGAME][r], PAWN_PASSED[CANNOT_ADVANCE][ENDGAME][r]);
			}
			else {
				if (!position.attacked(p + dir)) {
					pawn_structure[c] += getTaperedScore(PAWN_PASSED[SAFE_ADVANCE][MIDDLEGAME][r], PAWN_PASSED[SAFE_ADVANCE][ENDGAME][r]);
				}
				else {
					if (position.defended(p + dir, c)) {
						pawn_structure[c] += getTaperedScore(PAWN_PASSED[PROTECTED_ADVANCE][MIDDLEGAME][r], PAWN_PASSED[PROTECTED_ADVANCE][ENDGAME][r]);
					}
					else {
						pawn_structure[c] += getTaperedScore(PAWN_PASSED[UNSAFE_ADVANCE][MIDDLEGAME][r], PAWN_PASSED[UNSAFE_ADVANCE][ENDGAME][r]);
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
		material[c] += getTaperedScore(KNIGHT_WEIGHT_MG, KNIGHT_WEIGHT_EG);
		if (square_bb[p] & pins) {
			mobility[c] += getTaperedScore(KNIGHT_MOBILITY[MIDDLEGAME][0], KNIGHT_MOBILITY[ENDGAME][0]);
			continue;
		}
		moves = position.getAttackBB<PIECETYPE_KNIGHT>(p);
		piece_attacks_bb[c][PIECETYPE_KNIGHT] |= moves & position.getOccupancyBB();
		all_attacks_bb[c] |= piece_attacks_bb[c][PIECETYPE_KNIGHT];
		mobility[c] += getTaperedScore(KNIGHT_MOBILITY[MIDDLEGAME][pop_count(moves & mobilityNet)], KNIGHT_MOBILITY[ENDGAME][pop_count(moves & mobilityNet)]);
		if (moves & king_net_bb[!c][position.getKingSquare(!c)] & mobilityNet) {
			king_threats += KNIGHT_THREAT;
		}
	}

	for (Square p : position.getPieceList<PIECETYPE_BISHOP>(c)) {
		if (p == no_sq) {
			break;
		}
		material[c] += getTaperedScore(BISHOP_WEIGHT_MG, BISHOP_WEIGHT_EG);
		moves = position.getAttackBB<PIECETYPE_BISHOP>(p);
		if (square_bb[p] & pins) {
			moves &= coplanar[p][kingSq];
		}
		piece_attacks_bb[c][PIECETYPE_BISHOP] = moves & position.getOccupancyBB();
		all_attacks_bb[c] |= piece_attacks_bb[c][PIECETYPE_BISHOP];
		if (moves & king_net_bb[!c][position.getKingSquare(!c)] & mobilityNet) {
			king_threats += BISHOP_THREAT;
		}
		mobility[c] += getTaperedScore(BISHOP_MOBILITY[MIDDLEGAME][pop_count(moves & mobilityNet)], BISHOP_MOBILITY[ENDGAME][pop_count(moves & mobilityNet)]);
		material[c] += BAD_BISHOP * pop_count(squares_of_color(p) & position.getPieceBB<PIECETYPE_PAWN>(c));
	}
	
	for (Square p : position.getPieceList<PIECETYPE_ROOK>(c)) {
		if (p == no_sq) {
			break;
		}
		material[c] += getTaperedScore(ROOK_WEIGHT_MG, ROOK_WEIGHT_EG);
		moves = position.getAttackBB<PIECETYPE_ROOK>(p);
		if (square_bb[p] & pins) {
			moves &= coplanar[p][kingSq];
		}
		piece_attacks_bb[c][PIECETYPE_ROOK] = moves & position.getOccupancyBB();
		all_attacks_bb[c] |= piece_attacks_bb[c][PIECETYPE_ROOK];
		if (moves & king_net_bb[!c][position.getKingSquare(!c)] & mobilityNet) {
			king_threats += ROOK_THREAT;
		}
		mobility[c] += getTaperedScore(ROOK_MOBILITY[MIDDLEGAME][pop_count(moves & mobilityNet)], ROOK_MOBILITY[ENDGAME][pop_count(moves & mobilityNet)]);
	}
	
	for (Square p : position.getPieceList<PIECETYPE_QUEEN>(c)) {
		if (p == no_sq) {
			break;
		}
		material[c] += getTaperedScore(QUEEN_WEIGHT_MG, QUEEN_WEIGHT_EG);
		moves = position.getAttackBB<PIECETYPE_QUEEN>(p);
		if (square_bb[p] & pins) {
			moves &= coplanar[p][kingSq];
		}
		piece_attacks_bb[c][PIECETYPE_QUEEN] = moves & position.getOccupancyBB();
		all_attacks_bb[c] |= piece_attacks_bb[c][PIECETYPE_QUEEN];
		if (moves & king_net_bb[!c][position.getKingSquare(!c)] & mobilityNet) {
			king_threats += QUEEN_THREAT;
		}
		mobility[c] += getTaperedScore(QUEEN_MOBILITY[MIDDLEGAME][pop_count(moves & mobilityNet)], QUEEN_MOBILITY[ENDGAME][pop_count(moves & mobilityNet)]);
	}
	
	// Bishop pair
	if (position.getPieceCount<PIECETYPE_BISHOP>(c) >= 2) {
		material[c] += getTaperedScore(BISHOP_PAIR_MG, BISHOP_PAIR_EG);
	}

	// Threats on enemy King
	king_safety[!c] -= SAFETY_TABLE[king_threats];
}

void Evaluate::evalPawnShield(const Color c) {
	if (position.getPieceCount<PIECETYPE_ROOK>() + position.getPieceCount<PIECETYPE_QUEEN>() == 0) {
		return;
	}
	Square kingSq = position.getKingSquare(c);

	if (c == WHITE) {
		if (rank(kingSq) > 1) {
			return;
		}
		if (file(kingSq) <= 2) { // Kingside
			evalPawnShield(WHITE, square_bb[F2] | square_bb[G2] | square_bb[H2]);
		}
		else if (file(kingSq) >= 5) { // Queenside
			evalPawnShield(WHITE, square_bb[A2] | square_bb[B2] | square_bb[C2]);
		}
	}
	else {
		if (rank(kingSq) < 6) {
			return;
		}
		if (file(kingSq) <= 2) { // Kingside
			evalPawnShield(BLACK, square_bb[F7] | square_bb[G7] | square_bb[H7]);
		}
		else if (file(kingSq) >= 5) { // Queenside
			evalPawnShield(BLACK, square_bb[A7] | square_bb[B7] | square_bb[C7]);
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
	
	attacks[c] += pop_count(hanging) * HANGING;
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
		<< std::setw(12) << e.material[c] << " |"
		<< std::setw(12) << e.material[!c] << " |"
		<< std::setw(12) << e.material[c] - e.material[!c] << " |\n"
		<< "-------------------------------------------------------------\n"
		<< "| Mobility        |" 
		<< std::setw(12) << e.mobility[c] << " |" 
		<< std::setw(12) << e.mobility[!c] << " |"
		<< std::setw(12) << e.mobility[c] - e.mobility[!c] << " |\n"
		<< "-------------------------------------------------------------\n"
		<< "| Pawn Structure  |"
		<< std::setw(12) << e.pawn_structure[c] << " |" 
		<< std::setw(12) << e.pawn_structure[!c] << " |"
		<< std::setw(12) << e.pawn_structure[c] - e.pawn_structure[!c] 
		<< " |\n"
		<< "-------------------------------------------------------------\n"
		<< "| King Safety     |"
		<< std::setw(12) << e.king_safety[c] << " |" 
		<< std::setw(12) << e.king_safety[!c] << " |"
		<< std::setw(12) << e.king_safety[c] - e.king_safety[!c] << " |\n"
		<< "-------------------------------------------------------------\n"
		<< "| Attacks         |"
		<< std::setw(12) << e.attacks[c] << " |" 
		<< std::setw(12) << e.attacks[!c] << " |"
		<< std::setw(12) << e.attacks[c] - e.attacks[!c] << " |\n"
		<< "-------------------------------------------------------------\n"
		<< "| PST             |"
		<< std::setw(12) << pstMid << " |" 
		<< std::setw(12) << pstLate << " |"
		<< std::setw(12) << pstMid + pstLate << " |\n"
		<< "-------------------------------------------------------------\n"
		<< "| Total           |             |             |"
		<< std::setw(12) << e.score << " |\n"
		<< "-------------------------------------------------------------\n";

	return os;
}