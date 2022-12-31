#include "eval.h"

// Heavy or light eval?

Evaluate::Evaluate(const State& pState) : mState(pState) , mMaterial{}, mPawnStructure{}, mMobility{}, mKingSafety{}, mAttacks{}, mPieceAttacksBB{}, mAllAttacksBB{} {
	
	// Check for entry in pawn hash table
	if (mState.getPieceCount<pawn>()) {
		const PawnEntry* pawnEntry = probe(mState.getPawnKey());
		if (pawnEntry && pawnEntry->getKey() == mState.getPawnKey()) {
			mPawnStructure = pawnEntry->getStructure();
			mMaterial = pawnEntry->getMaterial();
		}
		else {
			evalPawns(WHITE);
			evalPawns(BLACK);
			store(mState.getPawnKey(), mPawnStructure, mMaterial);
		}
	}
	
	mGamePhase = mState.getGamePhase(); // Tapered-evaluation, game phase between 0 (Opening/Middlegame) and 255 (Endgame)
	evalPieces(WHITE);
	evalPieces(BLACK);

	// Pawn attacks
	mPieceAttacksBB[WHITE][pawn] |= (mState.getPieceBB<pawn>(WHITE) & NOT_A_FILE) << 9 & mState.getOccupancyBB();
	mPieceAttacksBB[WHITE][pawn] |= (mState.getPieceBB<pawn>(WHITE) & NOT_H_FILE) << 7 & mState.getOccupancyBB();
	mPieceAttacksBB[BLACK][pawn] |= (mState.getPieceBB<pawn>(BLACK) & NOT_A_FILE) >> 7 & mState.getOccupancyBB();
	mPieceAttacksBB[BLACK][pawn] |= (mState.getPieceBB<pawn>(BLACK) & NOT_H_FILE) >> 9 & mState.getOccupancyBB();
	mAllAttacksBB[WHITE] |= mPieceAttacksBB[WHITE][pawn];
	mAllAttacksBB[BLACK] |= mPieceAttacksBB[BLACK][pawn];

	evalAttacks(WHITE);
	evalAttacks(BLACK);

	Color c = mState.getOurColor();
	mScore = mMobility[c] - mMobility[!c] + mKingSafety[c] - mKingSafety[!c] + mPawnStructure[c] - mPawnStructure[!c] + mMaterial[c] - mMaterial[!c];
	mScore += ((mState.getPstScore(MIDDLEGAME) * (256 - mGamePhase)) + mState.getPstScore(ENDGAME) * mGamePhase) / 256;

    mScore += TEMPO_BONUS; // Side-to-move bonus
}

int Evaluate::getScore() const {
	return mScore;
}

template<PieceType PT> int Evaluate::outpost(Square p, Color c) {
    int score;
    if (!(p & outpost_area[c]) || !(pawn_attacks[!c][p] & mState.getPieceBB<pawn>(c)) || in_front[c][p] & adj_files[p] & mState.getPieceBB<pawn>(!c)) {
		return 0;
	}
	
	score = PieceSquareTable::getOutpostScore(PT, c, p);
	// Bonus if outpost can't be attacked by opponent minor piece
    if (!mState.getPieceBB<knight>(!c) && !(mState.getPieceBB<bishop>(!c) & squares_of_color(p))) {
		score += OUTPOST_BONUS;
	}
	return score;
}

void Evaluate::evalPawns(const Color c) {
	const int dir = c == WHITE ? 8 : -8;
	
	for (Square p : mState.getPieceList<pawn>(c)) {
		if (p == no_sq) {
			break;
		}

		mMaterial[c] += ((PAWN_WEIGHT_MG * (256 - mGamePhase)) + PAWN_WEIGHT_EG * mGamePhase) / 256;

		if (!((file_bb[p] | adj_files[p]) & in_front[c][p] & mState.getPieceBB<pawn>(!c))) {
			mPawnStructure[c] += PAWN_PASSED; // No pawns in adjacent files or in front
		}
		
		else if (!(file_bb[p] & in_front[c][p] & mState.getPieceBB<pawn>(!c))) {
			int sentries, helpers;
			assert(p < 56 && p > 7);

			sentries = pop_count(pawn_attacks[c][p + dir] & mState.getPieceBB<pawn>(!c));
			if (sentries > 0) {
				helpers = pop_count(pawn_attacks[!c][p + dir] & mState.getPieceBB<pawn>(c));
				if (helpers >= sentries) {
					mPawnStructure[c] += PAWN_PASSED_CANDIDATE;
				}
				else if (!(~in_front[c][p] & adj_files[p] & mState.getPieceBB<pawn>(c))) {
					if (pawn_attacks[c][p] & mState.getPieceBB<pawn>(c)) {
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

		if (!(adj_files[p] & mState.getPieceBB<pawn>(c))) {
			mPawnStructure[c] += PAWN_ISOLATED;
		}
		else if (rank_bb[p - dir] & mState.getPieceBB<pawn>(c) & adj_files[p]) {
			mPawnStructure[c] += PAWN_CONNECTED;
		}
		
		if (pop_count(file_bb[p] & mState.getPieceBB<pawn>(c)) > 1) {
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
		mobilityNet &= ~((mState.getPieceBB<pawn>(BLACK) & NOT_A_FILE) >> 7 | (mState.getPieceBB<pawn>(BLACK) & NOT_H_FILE) >> 9);
	}
	else {
		mobilityNet &= ~((mState.getPieceBB<pawn>(WHITE) & NOT_A_FILE) << 9	| (mState.getPieceBB<pawn>(WHITE) & NOT_H_FILE) << 7);
	}
	
	pins = mState.getPinsBB(c);

	for (Square p : mState.getPieceList<knight>(c)) {
		if (p == no_sq) {
			break;
		}
		mMaterial[c] += ((KNIGHT_WEIGHT_MG * (256 - mGamePhase)) + KNIGHT_WEIGHT_EG * mGamePhase) / 256;
		//mMaterial[c] += outpost<knight>(p, c);
		if (square_bb[p] & pins) {
			mMobility[c] += KNIGHT_MOBILITY[0];
			continue;
		}
		moves = mState.getAttackBB<knight>(p);
		mPieceAttacksBB[c][knight] |= moves & mState.getOccupancyBB();
		mAllAttacksBB[c] |= mPieceAttacksBB[c][knight];
		mMobility[c] += KNIGHT_MOBILITY[pop_count(moves & mobilityNet)];
		if (moves & king_net_bb[!c][mState.getKingSquare(!c)] & mobilityNet) {
			king_threats += KNIGHT_THREAT;
		}
	}

	for (Square p : mState.getPieceList<bishop>(c)) {
		if (p == no_sq) {
			break;
		}
		mMaterial[c] += ((BISHOP_WEIGHT_MG * (256 - mGamePhase)) + BISHOP_WEIGHT_EG * mGamePhase) / 256;
		//mMaterial[c] += outpost<bishop>(p, c);
		moves = mState.getAttackBB<bishop>(p);
		if (square_bb[p] & pins) {
			moves &= coplanar[p][kingSq];
		}
		mPieceAttacksBB[c][bishop] = moves & mState.getOccupancyBB();
		mAllAttacksBB[c] |= mPieceAttacksBB[c][bishop];
		if (moves & king_net_bb[!c][mState.getKingSquare(!c)] & mobilityNet) {
			king_threats += BISHOP_THREAT;
		}
		mMobility[c] += BISHOP_MOBILITY[pop_count(moves & mobilityNet)];
		mMaterial[c] += BAD_BISHOP * pop_count(squares_of_color(p) & mState.getPieceBB<pawn>(c));
	}

    for (Square p : mState.getPieceList<rook>(c)) {
		if (p == no_sq) {
			break;
		}
		mMaterial[c] += ((ROOK_WEIGHT_MG * (256 - mGamePhase)) + ROOK_WEIGHT_EG * mGamePhase) / 256;
		moves = mState.getAttackBB<rook>(p);
		if (square_bb[p] & pins) {
			moves &= coplanar[p][kingSq];
		}
		mPieceAttacksBB[c][rook] = moves & mState.getOccupancyBB();
		mAllAttacksBB[c] |= mPieceAttacksBB[c][rook];
		if (moves & king_net_bb[!c][mState.getKingSquare(!c)] & mobilityNet) {
			king_threats += ROOK_THREAT;
		}
		mMobility[c] += ROOK_MOBILITY[pop_count(moves & mobilityNet)];
		//if (!(file_bb[p] & in_front[c][p] & mState.getPieceBB<pawn>(!c))) {
		//	mMaterial[c] += ((ROOK_OPEN_FILE_MG * (256 - mGamePhase)) + ROOK_OPEN_FILE_EG * mGamePhase) / 256;
		//}
		if (mState.getPieceBB<king>(c) & bottomRank && square_bb[p] & bottomRank) { // **NOTE** check if this is working properly
			if ((kingSq > p && !mState.canCastleKingside(c) && square_bb[kingSq] & RIGHTSIDE) || (kingSq < p && !mState.canCastleQueenside(c) && square_bb[kingSq] & LEFTSIDE)) {
				if (pop_count(moves & mobilityNet & ~(bottomRank)) <= 3) {
					mMaterial[c] += TRAPPED_ROOK;
				}
			}
		}
	}

    for (Square p : mState.getPieceList<queen>(c)) {
		if (p == no_sq) {
			break;
		}
		mMaterial[c] += ((QUEEN_WEIGHT_MG * (256 - mGamePhase)) + QUEEN_WEIGHT_EG * mGamePhase) / 256;
		moves = mState.getAttackBB<queen>(p);
		if (square_bb[p] & pins) {
			moves &= coplanar[p][kingSq];
		}
		mPieceAttacksBB[c][queen] = moves & mState.getOccupancyBB();
		mAllAttacksBB[c] |= mPieceAttacksBB[c][queen];
		if (moves & king_net_bb[!c][mState.getKingSquare(!c)] & mobilityNet) {
			king_threats += QUEEN_THREAT;
		}
		mMobility[c] += QUEEN_MOBILITY[pop_count(moves & mobilityNet)];
	}
	
	// Remaining evaluation values
	// Bishop pair
	//if (mState.getPieceCount<bishop>(c) >= 2 && mState.getPieceCount<bishop>(!c) < 2) {
	//	mMaterial[c] += ((BISHOP_PAIR_MG * (256 - mGamePhase)) + BISHOP_PAIR_EG * mGamePhase) / 256;
	//}
	
	mKingSafety[!c] -= SAFETY_TABLE[king_threats];
}

void Evaluate::evalAttacks(Color c) {
	U64 attackedByPawn, hanging;
	attackedByPawn = mPieceAttacksBB[!c][pawn] & (mState.getOccupancyBB(c) ^ mState.getPieceBB<pawn>(c));

	while (attackedByPawn) {
		Square to = pop_lsb(attackedByPawn);
		if (mState.getAttackBB<pawn>(to, c) & mState.getPieceBB<pawn>(!c) & mAllAttacksBB[!c]) {
			mAttacks[c] += STRONG_PAWN_ATTACK;
		}
		else {
			mAttacks[c] += WEAK_PAWN_ATTACK;
		}
	}

	// Hanging pieces
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
		<< "| PST (mid/late)  |"
		<< std::setw(12) << pstMid << " |" 
		<< std::setw(12) << pstLate << " |"
		<< std::setw(12) << pstMid + pstLate << " |\n"
		<< "-------------------------------------------------------------\n"
		<< "| Total           |             |             |"
		<< std::setw(12) << e.mScore << " |\n"
		<< "-------------------------------------------------------------\n";

	return os;
}