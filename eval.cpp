/**
 * Moraband, known in antiquity as Korriban, was an 
 * Outer Rim planet that was home to the ancient Sith 
 **/

#include "eval.h"

PawnHashTable ptable;

/* Evaluation and related functions */
Evaluate::Evaluate(const State& s) : mState(s), mMaterial{}, mPawnStructure{}, mMobility{}, mKingSafety{}, mAttacks{}, mPieceAttacksBB{}, mAllAttacksBB{} {
	// Tapered-evaluation with 256 phases, 0 (Opening/Middlegame) and 255 (Endgame)
	mGamePhase = mState.getGamePhase();
	Color c = mState.getOurColor();
	// Check for entry in pawn hash table
	if (mState.getPieceCount<PIECETYPE_PAWN>()) {
		const PawnEntry* pawnEntry = ptable.probe(mState.getPawnKey());
		if (pawnEntry && pawnEntry->getKey() == mState.getPawnKey()) {
			mPawnStructure = pawnEntry->getStructure();
			mMaterial = pawnEntry->getMaterial();
		}
		else {
			evalPawns(WHITE);
			evalPawns(BLACK);
			ptable.store(mState.getPawnKey(), mPawnStructure, mMaterial);
		}
	}

	evalPieces(WHITE);
	evalPieces(BLACK);

	// Pawn attacks
	mPieceAttacksBB[WHITE][PIECETYPE_PAWN] |= (mState.getPieceBB<PIECETYPE_PAWN>(WHITE) & NOT_A_FILE) << 9 & mState.getOccupancyBB();
	mPieceAttacksBB[WHITE][PIECETYPE_PAWN] |= (mState.getPieceBB<PIECETYPE_PAWN>(WHITE) & NOT_H_FILE) << 7 & mState.getOccupancyBB();
	mPieceAttacksBB[BLACK][PIECETYPE_PAWN] |= (mState.getPieceBB<PIECETYPE_PAWN>(BLACK) & NOT_A_FILE) >> 7 & mState.getOccupancyBB();
	mPieceAttacksBB[BLACK][PIECETYPE_PAWN] |= (mState.getPieceBB<PIECETYPE_PAWN>(BLACK) & NOT_H_FILE) >> 9 & mState.getOccupancyBB();
	mAllAttacksBB[WHITE] |= mPieceAttacksBB[WHITE][PIECETYPE_PAWN];
	mAllAttacksBB[BLACK] |= mPieceAttacksBB[BLACK][PIECETYPE_PAWN];

	evalAttacks(WHITE);
	evalAttacks(BLACK);
	mScore = mMobility[c] - mMobility[!c] + mKingSafety[c] - mKingSafety[!c] + mPawnStructure[c] - mPawnStructure[!c] + mMaterial[c] - mMaterial[!c] + mAttacks[c] - mAttacks[!c];
	mScore += getTaperedScore(mState.getPstScore(MIDDLEGAME), mState.getPstScore(ENDGAME));
	mScore += TEMPO_BONUS;
}

int Evaluate::getScore() const {
	return mScore + CONTEMPT;
}

int Evaluate::getTaperedScore(int mg, int eg) {
	return (mg * (256 - mGamePhase) + eg * mGamePhase) / 256;
}

void Evaluate::evalOutposts(const Color c) {
	for (Square p : mState.getPieceList<PIECETYPE_KNIGHT>(c)) {
		if (p == no_sq) {
			break;
		}
		if (!(p & outpost_area[c]) || !(pawn_attacks[!c][p] & mState.getPieceBB<PIECETYPE_PAWN>(c)) || in_front[c][p] & adj_files[p] & mState.getPieceBB<PIECETYPE_PAWN>(!c)) {
			continue;
		}
		mMaterial[c] += KNIGHT_OUTPOST;
		if (!mState.getPieceBB<PIECETYPE_KNIGHT>(!c) && !mState.getPieceBB<PIECETYPE_BISHOP>(!c) & squares_of_color(p)) {
			mMaterial[c] += KNIGHT_OUTPOST;
		}
	}
	for (Square p : mState.getPieceList<PIECETYPE_BISHOP>(c)) {
		if (p == no_sq) {
			break;
		}
		if (!(p & outpost_area[c]) || !(pawn_attacks[!c][p] & mState.getPieceBB<PIECETYPE_PAWN>(c)) || in_front[c][p] & adj_files[p] & mState.getPieceBB<PIECETYPE_PAWN>(!c)) {
			continue;
		}
		mMaterial[c] += BISHOP_OUTPOST;
		if (!mState.getPieceBB<PIECETYPE_KNIGHT>(!c) && !mState.getPieceBB<PIECETYPE_BISHOP>(!c) & squares_of_color(p)) {
			mMaterial[c] += BISHOP_OUTPOST;
		}
	}
}

/* Evaluate pawn structure */
void Evaluate::evalPawns(const Color c) {
	const int dir = c == WHITE ? 8 : -8;

	for (Square p : mState.getPieceList<PIECETYPE_PAWN>(c)) {
		if (p == no_sq) {
			break;
		}
		int r = c == WHITE ? rank(p) : 8 - rank(p) - 1;

		mMaterial[c] += getTaperedScore(PAWN_WEIGHT_MG, PAWN_WEIGHT_EG);

		if (!((file_bb[p] | adj_files[p]) & in_front[c][p] & mState.getPieceBB<PIECETYPE_PAWN>(!c))) {
			mMaterial[c] += PAWN_PASSED;
		
			if ((rank_bb[p] | adj_ranks[p]) & in_front[c][p] & mState.getOccupancyBB()) {
				mPawnStructure[c] += getTaperedScore(PAWN_PASSED_ADVANCE[CANNOT_ADVANCE][MIDDLEGAME][r], PAWN_PASSED_ADVANCE[CANNOT_ADVANCE][ENDGAME][r]);
			}
			else {
				if (!mState.attacked(p + dir)) {
					mPawnStructure[c] += getTaperedScore(PAWN_PASSED_ADVANCE[SAFE_ADVANCE][MIDDLEGAME][r], PAWN_PASSED_ADVANCE[SAFE_ADVANCE][ENDGAME][r]);
				}
				else {
					if (mState.defended(p + dir, c)) {
						mPawnStructure[c] += getTaperedScore(PAWN_PASSED_ADVANCE[PROTECTED_ADVANCE][MIDDLEGAME][r], PAWN_PASSED_ADVANCE[PROTECTED_ADVANCE][ENDGAME][r]);
					}
					else {
						mPawnStructure[c] += getTaperedScore(PAWN_PASSED_ADVANCE[UNSAFE_ADVANCE][MIDDLEGAME][r], PAWN_PASSED_ADVANCE[UNSAFE_ADVANCE][ENDGAME][r]);
					}
				}
			}

		}
		else if (!(file_bb[p] & in_front[c][p] & mState.getPieceBB<PIECETYPE_PAWN>(!c))) {
			int sentries, helpers;
			assert(p < 56 && p > 7);

			sentries = pop_count(pawn_attacks[c][p + dir] & mState.getPieceBB<PIECETYPE_PAWN>(!c));
			if (sentries > 0) {
				helpers = pop_count(pawn_attacks[!c][p + dir] & mState.getPieceBB<PIECETYPE_PAWN>(c));
				if (helpers >= sentries) {
					mPawnStructure[c] += PAWN_PASSED_CANDIDATE;
				}
				else if (!(~in_front[c][p] & adj_files[p] & mState.getPieceBB<PIECETYPE_PAWN>(c))) {
					if (pawn_attacks[c][p] & mState.getPieceBB<PIECETYPE_PAWN>(c)) {
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

		if (!(adj_files[p] & mState.getPieceBB<PIECETYPE_PAWN>(c))) {
			mPawnStructure[c] += PAWN_ISOLATED;
		}
		else if (rank_bb[p - dir] & mState.getPieceBB<PIECETYPE_PAWN>(c) & adj_files[p]) {
			mPawnStructure[c] += PAWN_CONNECTED;
		}
		
		if (pop_count(file_bb[p] & mState.getPieceBB<PIECETYPE_PAWN>(c)) > 1) {
			mPawnStructure[c] += PAWN_DOUBLED;
		}
	}
}

void Evaluate::evalPieces(const Color c) {
	U64 moves, pins;
	U64 mobilityNet;
	int king_threats = 0;
	U64 bottomRank = c == WHITE ? RANK_1 : RANK_8;
	Square kingSq = mState.getKingSquare(c);

	mobilityNet = mState.getEmptyBB(); // All squares not attacked by pawns
	if (c == WHITE) {
		mobilityNet &= ~((mState.getPieceBB<PIECETYPE_PAWN>(BLACK) & NOT_A_FILE) >> 7 | (mState.getPieceBB<PIECETYPE_PAWN>(BLACK) & NOT_H_FILE) >> 9);
	}
	else {
		mobilityNet &= ~((mState.getPieceBB<PIECETYPE_PAWN>(WHITE) & NOT_A_FILE) << 9 | (mState.getPieceBB<PIECETYPE_PAWN>(WHITE) & NOT_H_FILE) << 7);
	}
	
	pins = mState.getPinsBB(c);

	for (Square p : mState.getPieceList<PIECETYPE_KNIGHT>(c)) {
		if (p == no_sq) {
			break;
		}
		mMaterial[c] += getTaperedScore(KNIGHT_WEIGHT_MG, KNIGHT_WEIGHT_EG);
		if (square_bb[p] & pins) {
			mMobility[c] += getTaperedScore(KNIGHT_MOBILITY[MIDDLEGAME][0], KNIGHT_MOBILITY[ENDGAME][0]);
			continue;
		}
		moves = mState.getAttackBB<PIECETYPE_KNIGHT>(p);
		mPieceAttacksBB[c][PIECETYPE_KNIGHT] |= moves & mState.getOccupancyBB();
		mAllAttacksBB[c] |= mPieceAttacksBB[c][PIECETYPE_KNIGHT];
		mMobility[c] += getTaperedScore(KNIGHT_MOBILITY[MIDDLEGAME][pop_count(moves & mobilityNet)], KNIGHT_MOBILITY[ENDGAME][pop_count(moves & mobilityNet)]);
		if (moves & king_net_bb[!c][mState.getKingSquare(!c)] & mobilityNet) {
			king_threats += KNIGHT_THREAT;
		}
	}

	for (Square p : mState.getPieceList<PIECETYPE_BISHOP>(c)) {
		if (p == no_sq) {
			break;
		}
		mMaterial[c] += getTaperedScore(BISHOP_WEIGHT_MG, BISHOP_WEIGHT_EG);
		moves = mState.getAttackBB<PIECETYPE_BISHOP>(p);
		if (square_bb[p] & pins) {
			moves &= coplanar[p][kingSq];
		}
		mPieceAttacksBB[c][PIECETYPE_BISHOP] = moves & mState.getOccupancyBB();
		mAllAttacksBB[c] |= mPieceAttacksBB[c][PIECETYPE_BISHOP];
		if (moves & king_net_bb[!c][mState.getKingSquare(!c)] & mobilityNet) {
			king_threats += BISHOP_THREAT;
		}
		mMobility[c] += getTaperedScore(BISHOP_MOBILITY[MIDDLEGAME][pop_count(moves & mobilityNet)], BISHOP_MOBILITY[ENDGAME][pop_count(moves & mobilityNet)]);
		mMaterial[c] += BAD_BISHOP * pop_count(squares_of_color(p) & mState.getPieceBB<PIECETYPE_PAWN>(c));
	}
	
	for (Square p : mState.getPieceList<PIECETYPE_ROOK>(c)) {
		if (p == no_sq) {
			break;
		}
		mMaterial[c] += getTaperedScore(ROOK_WEIGHT_MG, ROOK_WEIGHT_EG);
		moves = mState.getAttackBB<PIECETYPE_ROOK>(p);
		if (square_bb[p] & pins) {
			moves &= coplanar[p][kingSq];
		}
		mPieceAttacksBB[c][PIECETYPE_ROOK] = moves & mState.getOccupancyBB();
		mAllAttacksBB[c] |= mPieceAttacksBB[c][PIECETYPE_ROOK];
		if (moves & king_net_bb[!c][mState.getKingSquare(!c)] & mobilityNet) {
			king_threats += ROOK_THREAT;
		}
		mMobility[c] += getTaperedScore(ROOK_MOBILITY[MIDDLEGAME][pop_count(moves & mobilityNet)], ROOK_MOBILITY[ENDGAME][pop_count(moves & mobilityNet)]);
		// Trapped rooks
		/*
		if (mState.getPieceBB<PIECETYPE_KING>(c) & bottomRank && square_bb[p] & bottomRank) { // **NOTE** check if this is working properly
			if ((kingSq > p && !mState.canCastleKingside(c) && square_bb[kingSq] & RIGHTSIDE) || (kingSq < p && !mState.canCastleQueenside(c) && square_bb[kingSq] & LEFTSIDE)) {
				if (pop_count(moves & mobilityNet & ~(bottomRank)) <= 3) {
					mMaterial[c] += TRAPPED_ROOK;
				}
			}
		}
		*/
	}
	
	for (Square p : mState.getPieceList<PIECETYPE_QUEEN>(c)) {
		if (p == no_sq) {
			break;
		}
		mMaterial[c] += getTaperedScore(QUEEN_WEIGHT_MG, QUEEN_WEIGHT_EG);
		moves = mState.getAttackBB<PIECETYPE_QUEEN>(p);
		if (square_bb[p] & pins) {
			moves &= coplanar[p][kingSq];
		}
		mPieceAttacksBB[c][PIECETYPE_QUEEN] = moves & mState.getOccupancyBB();
		mAllAttacksBB[c] |= mPieceAttacksBB[c][PIECETYPE_QUEEN];
		if (moves & king_net_bb[!c][mState.getKingSquare(!c)] & mobilityNet) {
			king_threats += QUEEN_THREAT;
		}
		mMobility[c] += getTaperedScore(QUEEN_MOBILITY[MIDDLEGAME][pop_count(moves & mobilityNet)], QUEEN_MOBILITY[ENDGAME][pop_count(moves & mobilityNet)]);
	}
	
	// Bishop pair
	if (mState.getPieceCount<PIECETYPE_BISHOP>(c) >= 2) {
		mMaterial[c] += getTaperedScore(BISHOP_PAIR_MG, BISHOP_PAIR_EG);
	}
	
	mKingSafety[!c] -= SAFETY_TABLE[king_threats];
}

void Evaluate::evalAttacks(const Color c) {
	U64 attackedByPawn, hanging;
	attackedByPawn = mPieceAttacksBB[!c][PIECETYPE_PAWN] & (mState.getOccupancyBB(c) ^ mState.getPieceBB<PIECETYPE_PAWN>(c));

	while (attackedByPawn) {
		Square to = pop_lsb(attackedByPawn);
		if (mState.getAttackBB<PIECETYPE_PAWN>(to, c) & mState.getPieceBB<PIECETYPE_PAWN>(!c) & mAllAttacksBB[!c]) {
			mAttacks[c] += STRONG_PAWN_ATTACK;
		}
		else {
			mAttacks[c] += WEAK_PAWN_ATTACK;
		}
	}

	hanging = mAllAttacksBB[!c] & mState.getOccupancyBB(c) & ~mAllAttacksBB[c];
	
	mAttacks[c] += pop_count(hanging) * HANGING;
}

std::ostream& operator<<(std::ostream& os, const Evaluate& e) {
	Color c = e.mState.getOurColor();
	std::string us = c == WHITE ? "White" : "Black";
	std::string them = c == WHITE ? "Black" : "White";
	int pstMid = e.mState.getPstScore(MIDDLEGAME) * (256 - e.mGamePhase) / 256;
	int pstLate = e.mState.getPstScore(ENDGAME) * e.mGamePhase / 256;

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