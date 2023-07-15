/**
 * Moraband, known in antiquity as Korriban, was an 
 * Outer Rim planet that was home to the ancient Sith 
 **/

#include "eval.h"

int TEMPO_BONUS = 16; // Side-to-move bonus

int KNIGHT_THREAT = 6; // Threats on enemy King
int BISHOP_THREAT = 4;
int ROOK_THREAT = 7;
int QUEEN_THREAT = 10;

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
int KNIGHT_OUTPOST = 55;
int BISHOP_OUTPOST = 25;

PawnHashTable ptable;

/* Evaluation and related functions */
Evaluate::Evaluate(const Position& s) : mPosition(s), mMaterial{}, mPawnStructure{}, mMobility{}, mKingSafety{}, mAttacks{}, mPieceAttacksBB{}, mAllAttacksBB{} {
	// Tapered-evaluation with 256 phases, 0 (Opening/Middlegame) and 255 (Endgame)
	mGamePhase = mPosition.getGamePhase();
	Color c = mPosition.getOurColor();
	// Check for entry in pawn hash table
	if (mPosition.getPieceCount<PIECETYPE_PAWN>()) {
		const PawnEntry* pawnEntry = ptable.probe(mPosition.getPawnKey());
		if (pawnEntry && pawnEntry->getKey() == mPosition.getPawnKey()) {
			mPawnStructure = pawnEntry->getStructure();
			mMaterial = pawnEntry->getMaterial();
		}
		else {
			evalPawns(WHITE);
			evalPawns(BLACK);
			ptable.store(mPosition.getPawnKey(), mPawnStructure, mMaterial);
		}
	}

	evalPieces(WHITE);
	evalPieces(BLACK);

	// Pawn attacks
	mPieceAttacksBB[WHITE][PIECETYPE_PAWN] |= (mPosition.getPieceBB<PIECETYPE_PAWN>(WHITE) & NOT_A_FILE) << 9 & mPosition.getOccupancyBB();
	mPieceAttacksBB[WHITE][PIECETYPE_PAWN] |= (mPosition.getPieceBB<PIECETYPE_PAWN>(WHITE) & NOT_H_FILE) << 7 & mPosition.getOccupancyBB();
	mPieceAttacksBB[BLACK][PIECETYPE_PAWN] |= (mPosition.getPieceBB<PIECETYPE_PAWN>(BLACK) & NOT_A_FILE) >> 7 & mPosition.getOccupancyBB();
	mPieceAttacksBB[BLACK][PIECETYPE_PAWN] |= (mPosition.getPieceBB<PIECETYPE_PAWN>(BLACK) & NOT_H_FILE) >> 9 & mPosition.getOccupancyBB();
	mAllAttacksBB[WHITE] |= mPieceAttacksBB[WHITE][PIECETYPE_PAWN];
	mAllAttacksBB[BLACK] |= mPieceAttacksBB[BLACK][PIECETYPE_PAWN];

	evalAttacks(WHITE);
	evalAttacks(BLACK);
	mScore = mMobility[c] - mMobility[!c] + mKingSafety[c] - mKingSafety[!c] + mPawnStructure[c] - mPawnStructure[!c] + mMaterial[c] - mMaterial[!c] + mAttacks[c] - mAttacks[!c];
	mScore += getTaperedScore(mPosition.getPstScore(MIDDLEGAME), mPosition.getPstScore(ENDGAME));
	mScore += TEMPO_BONUS;
}

int Evaluate::getScore() const {
	return mScore + CONTEMPT;
}

int Evaluate::getTaperedScore(int mg, int eg) {
	return (mg * (256 - mGamePhase) + eg * mGamePhase) / 256;
}

void Evaluate::evalOutposts(const Color c) {
	for (Square p : mPosition.getPieceList<PIECETYPE_KNIGHT>(c)) {
		if (p == no_sq) {
			break;
		}
		if (!(p & outpost_area[c]) || !(pawn_attacks[!c][p] & mPosition.getPieceBB<PIECETYPE_PAWN>(c)) || in_front[c][p] & adj_files[p] & mPosition.getPieceBB<PIECETYPE_PAWN>(!c)) {
			continue;
		}
		mMaterial[c] += KNIGHT_OUTPOST;
		if (!mPosition.getPieceBB<PIECETYPE_KNIGHT>(!c) && !mPosition.getPieceBB<PIECETYPE_BISHOP>(!c) & squares_of_color(p)) {
			mMaterial[c] += KNIGHT_OUTPOST;
		}
	}
	for (Square p : mPosition.getPieceList<PIECETYPE_BISHOP>(c)) {
		if (p == no_sq) {
			break;
		}
		if (!(p & outpost_area[c]) || !(pawn_attacks[!c][p] & mPosition.getPieceBB<PIECETYPE_PAWN>(c)) || in_front[c][p] & adj_files[p] & mPosition.getPieceBB<PIECETYPE_PAWN>(!c)) {
			continue;
		}
		mMaterial[c] += BISHOP_OUTPOST;
		if (!mPosition.getPieceBB<PIECETYPE_KNIGHT>(!c) && !mPosition.getPieceBB<PIECETYPE_BISHOP>(!c) & squares_of_color(p)) {
			mMaterial[c] += BISHOP_OUTPOST;
		}
	}
}

/* Evaluate pawn structure */
void Evaluate::evalPawns(const Color c) {
	const int dir = c == WHITE ? 8 : -8;

	for (Square p : mPosition.getPieceList<PIECETYPE_PAWN>(c)) {
		if (p == no_sq) {
			break;
		}
		int r = c == WHITE ? rank(p) : 8 - rank(p) - 1;

		mMaterial[c] += getTaperedScore(PAWN_WEIGHT_MG, PAWN_WEIGHT_EG);

		if (!((file_bb[p] | adj_files[p]) & in_front[c][p] & mPosition.getPieceBB<PIECETYPE_PAWN>(!c))) {
			if ((p + dir) & mPosition.getOccupancyBB()) {
				mPawnStructure[c] += getTaperedScore(PAWN_PASSED[CANNOT_ADVANCE][MIDDLEGAME][r], PAWN_PASSED[CANNOT_ADVANCE][ENDGAME][r]);
			}
			else {
				if (!mPosition.attacked(p + dir)) {
					mPawnStructure[c] += getTaperedScore(PAWN_PASSED[SAFE_ADVANCE][MIDDLEGAME][r], PAWN_PASSED[SAFE_ADVANCE][ENDGAME][r]);
				}
				else {
					if (mPosition.defended(p + dir, c)) {
						mPawnStructure[c] += getTaperedScore(PAWN_PASSED[PROTECTED_ADVANCE][MIDDLEGAME][r], PAWN_PASSED[PROTECTED_ADVANCE][ENDGAME][r]);
					}
					else {
						mPawnStructure[c] += getTaperedScore(PAWN_PASSED[UNSAFE_ADVANCE][MIDDLEGAME][r], PAWN_PASSED[UNSAFE_ADVANCE][ENDGAME][r]);
					}
				}
			}
		}
		else if (!(file_bb[p] & in_front[c][p] & mPosition.getPieceBB<PIECETYPE_PAWN>(!c))) {
			int sentries, helpers;
			assert(p < 56 && p > 7);

			sentries = pop_count(pawn_attacks[c][p + dir] & mPosition.getPieceBB<PIECETYPE_PAWN>(!c));
			if (sentries > 0) {
				helpers = pop_count(pawn_attacks[!c][p + dir] & mPosition.getPieceBB<PIECETYPE_PAWN>(c));
				if (helpers >= sentries) {
					mPawnStructure[c] += PAWN_PASSED_CANDIDATE;
				}
				else if (!(~in_front[c][p] & adj_files[p] & mPosition.getPieceBB<PIECETYPE_PAWN>(c))) {
					if (pawn_attacks[c][p] & mPosition.getPieceBB<PIECETYPE_PAWN>(c)) {
						if (sentries == 2) {
							mPawnStructure[c] += PAWN_FULL_BACKWARDS;
						}
						else {
							mPawnStructure[c] += PAWN_BACKWARDS;
						}
					}
				}
			}
		}

		if (!(adj_files[p] & mPosition.getPieceBB<PIECETYPE_PAWN>(c))) {
			mPawnStructure[c] += PAWN_ISOLATED;
		}
		else if (rank_bb[p - dir] & mPosition.getPieceBB<PIECETYPE_PAWN>(c) & adj_files[p]) {
			mPawnStructure[c] += PAWN_CONNECTED;
		}
		
		if (pop_count(file_bb[p] & mPosition.getPieceBB<PIECETYPE_PAWN>(c)) > 1) {
			mPawnStructure[c] += PAWN_DOUBLED;
		}
	}
}

void Evaluate::evalPieces(const Color c) {
	U64 moves, pins;
	U64 mobilityNet;
	int king_threats = 0;
	U64 bottomRank = c == WHITE ? RANK_1 : RANK_8;
	Square kingSq = mPosition.getKingSquare(c);

	mobilityNet = mPosition.getEmptyBB(); // All squares not attacked by pawns
	if (c == WHITE) {
		mobilityNet &= ~((mPosition.getPieceBB<PIECETYPE_PAWN>(BLACK) & NOT_A_FILE) >> 7 | (mPosition.getPieceBB<PIECETYPE_PAWN>(BLACK) & NOT_H_FILE) >> 9);
	}
	else {
		mobilityNet &= ~((mPosition.getPieceBB<PIECETYPE_PAWN>(WHITE) & NOT_A_FILE) << 9 | (mPosition.getPieceBB<PIECETYPE_PAWN>(WHITE) & NOT_H_FILE) << 7);
	}
	
	pins = mPosition.getPinsBB(c);

	for (Square p : mPosition.getPieceList<PIECETYPE_KNIGHT>(c)) {
		if (p == no_sq) {
			break;
		}
		mMaterial[c] += getTaperedScore(KNIGHT_WEIGHT_MG, KNIGHT_WEIGHT_EG);
		if (square_bb[p] & pins) {
			mMobility[c] += getTaperedScore(KNIGHT_MOBILITY[MIDDLEGAME][0], KNIGHT_MOBILITY[ENDGAME][0]);
			continue;
		}
		moves = mPosition.getAttackBB<PIECETYPE_KNIGHT>(p);
		mPieceAttacksBB[c][PIECETYPE_KNIGHT] |= moves & mPosition.getOccupancyBB();
		mAllAttacksBB[c] |= mPieceAttacksBB[c][PIECETYPE_KNIGHT];
		mMobility[c] += getTaperedScore(KNIGHT_MOBILITY[MIDDLEGAME][pop_count(moves & mobilityNet)], KNIGHT_MOBILITY[ENDGAME][pop_count(moves & mobilityNet)]);
		if (moves & king_net_bb[!c][mPosition.getKingSquare(!c)] & mobilityNet) {
			king_threats += KNIGHT_THREAT;
		}
	}

	for (Square p : mPosition.getPieceList<PIECETYPE_BISHOP>(c)) {
		if (p == no_sq) {
			break;
		}
		mMaterial[c] += getTaperedScore(BISHOP_WEIGHT_MG, BISHOP_WEIGHT_EG);
		moves = mPosition.getAttackBB<PIECETYPE_BISHOP>(p);
		if (square_bb[p] & pins) {
			moves &= coplanar[p][kingSq];
		}
		mPieceAttacksBB[c][PIECETYPE_BISHOP] = moves & mPosition.getOccupancyBB();
		mAllAttacksBB[c] |= mPieceAttacksBB[c][PIECETYPE_BISHOP];
		if (moves & king_net_bb[!c][mPosition.getKingSquare(!c)] & mobilityNet) {
			king_threats += BISHOP_THREAT;
		}
		mMobility[c] += getTaperedScore(BISHOP_MOBILITY[MIDDLEGAME][pop_count(moves & mobilityNet)], BISHOP_MOBILITY[ENDGAME][pop_count(moves & mobilityNet)]);
		mMaterial[c] += BAD_BISHOP * pop_count(squares_of_color(p) & mPosition.getPieceBB<PIECETYPE_PAWN>(c));
	}
	
	for (Square p : mPosition.getPieceList<PIECETYPE_ROOK>(c)) {
		if (p == no_sq) {
			break;
		}
		mMaterial[c] += getTaperedScore(ROOK_WEIGHT_MG, ROOK_WEIGHT_EG);
		moves = mPosition.getAttackBB<PIECETYPE_ROOK>(p);
		if (square_bb[p] & pins) {
			moves &= coplanar[p][kingSq];
		}
		mPieceAttacksBB[c][PIECETYPE_ROOK] = moves & mPosition.getOccupancyBB();
		mAllAttacksBB[c] |= mPieceAttacksBB[c][PIECETYPE_ROOK];
		if (moves & king_net_bb[!c][mPosition.getKingSquare(!c)] & mobilityNet) {
			king_threats += ROOK_THREAT;
		}
		mMobility[c] += getTaperedScore(ROOK_MOBILITY[MIDDLEGAME][pop_count(moves & mobilityNet)], ROOK_MOBILITY[ENDGAME][pop_count(moves & mobilityNet)]);
		// Trapped rooks
		/*
		if (mPosition.getPieceBB<PIECETYPE_KING>(c) & bottomRank && square_bb[p] & bottomRank) { // **NOTE** check if this is working properly
			if ((kingSq > p && !mPosition.canCastleKingside(c) && square_bb[kingSq] & RIGHTSIDE) || (kingSq < p && !mPosition.canCastleQueenside(c) && square_bb[kingSq] & LEFTSIDE)) {
				if (pop_count(moves & mobilityNet & ~(bottomRank)) <= 3) {
					mMaterial[c] += TRAPPED_ROOK;
				}
			}
		}
		*/
	}
	
	for (Square p : mPosition.getPieceList<PIECETYPE_QUEEN>(c)) {
		if (p == no_sq) {
			break;
		}
		mMaterial[c] += getTaperedScore(QUEEN_WEIGHT_MG, QUEEN_WEIGHT_EG);
		moves = mPosition.getAttackBB<PIECETYPE_QUEEN>(p);
		if (square_bb[p] & pins) {
			moves &= coplanar[p][kingSq];
		}
		mPieceAttacksBB[c][PIECETYPE_QUEEN] = moves & mPosition.getOccupancyBB();
		mAllAttacksBB[c] |= mPieceAttacksBB[c][PIECETYPE_QUEEN];
		if (moves & king_net_bb[!c][mPosition.getKingSquare(!c)] & mobilityNet) {
			king_threats += QUEEN_THREAT;
		}
		mMobility[c] += getTaperedScore(QUEEN_MOBILITY[MIDDLEGAME][pop_count(moves & mobilityNet)], QUEEN_MOBILITY[ENDGAME][pop_count(moves & mobilityNet)]);
	}
	
	// Bishop pair
	if (mPosition.getPieceCount<PIECETYPE_BISHOP>(c) >= 2) {
		mMaterial[c] += getTaperedScore(BISHOP_PAIR_MG, BISHOP_PAIR_EG);
	}
	
	mKingSafety[!c] -= SAFETY_TABLE[king_threats];
}

void Evaluate::evalAttacks(const Color c) {
	U64 attackedByPawn, hanging;
	attackedByPawn = mPieceAttacksBB[!c][PIECETYPE_PAWN] & (mPosition.getOccupancyBB(c) ^ mPosition.getPieceBB<PIECETYPE_PAWN>(c));

	while (attackedByPawn) {
		Square to = pop_lsb(attackedByPawn);
		if (mPosition.getAttackBB<PIECETYPE_PAWN>(to, c) & mPosition.getPieceBB<PIECETYPE_PAWN>(!c) & mAllAttacksBB[!c]) {
			mAttacks[c] += STRONG_PAWN_ATTACK;
		}
		else {
			mAttacks[c] += WEAK_PAWN_ATTACK;
		}
	}

	hanging = mAllAttacksBB[!c] & mPosition.getOccupancyBB(c) & ~mAllAttacksBB[c];
	
	mAttacks[c] += pop_count(hanging) * HANGING;
}

std::ostream& operator<<(std::ostream& os, const Evaluate& e) {
	Color c = e.mPosition.getOurColor();
	std::string us = c == WHITE ? "White" : "Black";
	std::string them = c == WHITE ? "Black" : "White";
	int pstMid = e.mPosition.getPstScore(MIDDLEGAME) * (256 - e.mGamePhase) / 256;
	int pstLate = e.mPosition.getPstScore(ENDGAME) * e.mGamePhase / 256;

	os  << "-------------------------------------------------------------\n"
		<< "| Evaluation Type |    " << us << "    |    " << them 
		<< "    |    Total    |\n"
		<< "-------------------------------------------------------------\n"
		<< "| Material        |"
		<< std::setw(12) << e.mMaterial[c] << " |"
		<< std::setw(12) << e.mMaterial[!c] << " |"
		<< std::setw(12) << e.mMaterial[c] - e.mMaterial[!c] << " |\n"
		<< "-------------------------------------------------------------\n"
		<< "| Mobility        |" 
		<< std::setw(12) << e.mMobility[c] << " |" 
		<< std::setw(12) << e.mMobility[!c] << " |"
		<< std::setw(12) << e.mMobility[c] - e.mMobility[!c] << " |\n"
		<< "-------------------------------------------------------------\n"
		<< "| Pawn Structure  |"
		<< std::setw(12) << e.mPawnStructure[c] << " |" 
		<< std::setw(12) << e.mPawnStructure[!c] << " |"
		<< std::setw(12) << e.mPawnStructure[c] - e.mPawnStructure[!c] 
		<< " |\n"
		<< "-------------------------------------------------------------\n"
		<< "| King Safety     |"
		<< std::setw(12) << e.mKingSafety[c] << " |" 
		<< std::setw(12) << e.mKingSafety[!c] << " |"
		<< std::setw(12) << e.mKingSafety[c] - e.mKingSafety[!c] << " |\n"
		<< "-------------------------------------------------------------\n"
		<< "| Attacks         |"
		<< std::setw(12) << e.mAttacks[c] << " |" 
		<< std::setw(12) << e.mAttacks[!c] << " |"
		<< std::setw(12) << e.mAttacks[c] - e.mAttacks[!c] << " |\n"
		<< "-------------------------------------------------------------\n"
		<< "| PST             |"
		<< std::setw(12) << pstMid << " |" 
		<< std::setw(12) << pstLate << " |"
		<< std::setw(12) << pstMid + pstLate << " |\n"
		<< "-------------------------------------------------------------\n"
		<< "| Total           |             |             |"
		<< std::setw(12) << e.mScore << " |\n"
		<< "-------------------------------------------------------------\n";

	return os;
}