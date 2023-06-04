/**
 * Moraband, known in antiquity as Korriban, was an 
 * Outer Rim planet that was home to the ancient Sith 
 **/

#include "movegen.h"

/* Initialize magic moves */
void mg_init() {
	initmagicmoves();
}

/* List of moves and related functions */
MoveList::MoveList(const State& s, Move bestMove, History* history, int ply, bool qsearch) 
: mState(s), mValid(FULL), mBest(bestMove), mQSearch(qsearch), mKiller1(NULL_MOVE), mKiller2(NULL_MOVE) , mSize(0), mHistory(history), mPly(ply) {
	if (history) {
		mKiller1 = history->getKiller(ply).first;
		mKiller2 = history->getKiller(ply).second;
	}
	if (mState.inCheck()) {
		Square k = mState.getKingSquare(mState.getOurColor());
		Square c = get_lsb(mState.getCheckersBB());
		mValid = between[k][c] | mState.getCheckersBB();
		mStage = EvadeBestMove;
	}
	else {
		mStage = qsearch ? QBestMove : BestMove;
	}
}

std::size_t MoveList::size() const {
	return mSize;
}

void MoveList::push(Move move) {
	mList[mSize++].move = move;
}

Move MoveList::pop() {
	return mList[--mSize].move;
}

bool MoveList::contains(Move move) const {
	return std::find(mList.begin(), mList.begin() + mSize, move) != mList.begin() + mSize;
}

template<MoveType T> void MoveList::pushPromotion(Square src, Square dst) {
	const Color C = mState.getOurColor();
	if (square_bb[dst] & NOT_A_FILE && square_bb[dst + 1] & mState.getOccupancyBB(!C)) {
		if (T == MoveType::Attacks || T == MoveType::All) {
			push(makeMove(src, dst + 1, PIECETYPE_QUEEN));
		}
		if (T == MoveType::Quiets || T == MoveType::All) {
			push(makeMove(src, dst + 1, PIECETYPE_KNIGHT));
			push(makeMove(src, dst + 1, PIECETYPE_ROOK));
			push(makeMove(src, dst + 1, PIECETYPE_BISHOP));
		}
		if (T == MoveType::Evasions) {
			if (square_bb[dst + 1] & mValid) {
				push(makeMove(src, dst + 1, PIECETYPE_QUEEN));
				push(makeMove(src, dst + 1, PIECETYPE_KNIGHT));
				push(makeMove(src, dst + 1, PIECETYPE_ROOK));
				push(makeMove(src, dst + 1, PIECETYPE_BISHOP));
			}
		}
		if (T == MoveType::QuietChecks)	{
			if (mState.getCheckSquaresBB(PIECETYPE_KNIGHT) & square_bb[dst + 1]) {
				push(makeMove(src, dst + 1, PIECETYPE_KNIGHT));
			}
		}
	}

	if (square_bb[dst] & NOT_H_FILE && square_bb[dst - 1] & mState.getOccupancyBB(!C)) {
		if (T == MoveType::Attacks || T == MoveType::All) {
			push(makeMove(src, dst - 1, PIECETYPE_QUEEN));
		}
		if (T == MoveType::Quiets || T == MoveType::All) {
			push(makeMove(src, dst - 1, PIECETYPE_KNIGHT));
			push(makeMove(src, dst - 1, PIECETYPE_ROOK));
			push(makeMove(src, dst - 1, PIECETYPE_BISHOP));
		}
		if (T == MoveType::Evasions) {
			if (square_bb[dst - 1] & mValid) {
				push(makeMove(src, dst - 1, PIECETYPE_QUEEN));
				push(makeMove(src, dst - 1, PIECETYPE_KNIGHT));
				push(makeMove(src, dst - 1, PIECETYPE_ROOK));
				push(makeMove(src, dst - 1, PIECETYPE_BISHOP));
			}
		}
		if (T == MoveType::QuietChecks) {
			if (mState.getCheckSquaresBB(PIECETYPE_KNIGHT) & square_bb[dst - 1]) {
				push(makeMove(src, dst - 1, PIECETYPE_KNIGHT));
			}
		}
	}

	if (square_bb[dst] & mState.getEmptyBB()) {
		if (T == MoveType::Attacks || T == MoveType::All) {
			push(makeMove(src, dst, PIECETYPE_QUEEN));
		}
		if (T == MoveType::Quiets || T == MoveType::All) {
			push(makeMove(src, dst, PIECETYPE_KNIGHT));
			push(makeMove(src, dst, PIECETYPE_ROOK));
			push(makeMove(src, dst, PIECETYPE_BISHOP));
		}
		if (T == MoveType::Evasions) {
			if (square_bb[dst] & mValid) {
				push(makeMove(src, dst, PIECETYPE_QUEEN));
				push(makeMove(src, dst, PIECETYPE_KNIGHT));
				push(makeMove(src, dst, PIECETYPE_ROOK));
				push(makeMove(src, dst, PIECETYPE_BISHOP));
			}
		}
		if (T == MoveType::QuietChecks) {
			if (mState.getCheckSquaresBB(PIECETYPE_KNIGHT) & square_bb[dst]) {
				push(makeMove(src, dst, PIECETYPE_KNIGHT));
			}
		}
	}
}

template<MoveType T, PieceType P> void MoveList::pushMoves() {
	U64 m;
	Square dst;
	Color c = mState.getOurColor();

	for (Square src : mState.getPieceList<P>(c)) {
		if (src == no_sq) {
			break;
		}
		m = mState.getAttackBB<P>(src) & mValid;

		if (T == MoveType::QuietChecks) {
			if (square_bb[src] & mDiscover) {
				if (P == PIECETYPE_KING) {
					m &= ~coplanar[src][mState.getKingSquare(!c)];
				}
			}
			else {
				m &= mState.getCheckSquaresBB(P);
			}
		}

		while (m) {
			push(makeMove(src, pop_lsb(m)));
		}

		if (P == PIECETYPE_KING && T != MoveType::Attacks && T != MoveType::Evasions) {
			pushCastle<T>();
		}
	}
}

template<MoveType T, Color C> void MoveList::pushPawnMoves() {
	constexpr int U = C == WHITE ? 8 : -8;
	constexpr int L = C == WHITE ? 9 : -7;
	constexpr int R = C == WHITE ? 7 : -9;

	U64 promo, pawns;
	Square dst;
	pawns = mState.getPieceBB<PIECETYPE_PAWN>(C);
	promo = (C == WHITE ? pawns << 8 : pawns >> 8) & RANK_PROMOTION;

	while (promo) {
		dst = pop_lsb(promo);
		pushPromotion<T>(dst - U, dst);
	}

	if (T == MoveType::Attacks || T == MoveType::Evasions || T == MoveType::All) {
		U64 occ = ((T == MoveType::Evasions ? mState.getOccupancyBB(!C) & mValid
			: mState.getOccupancyBB(!C))
				| mState.getEnPassantBB()) & ~RANK_PROMOTION;

		U64 left  = C == WHITE ? (pawns & NOT_A_FILE) << 9 & occ
			: (pawns & NOT_A_FILE) >> 7 & occ;
		U64 right = C == WHITE ? (pawns & NOT_H_FILE) << 7 & occ
			: (pawns & NOT_H_FILE) >> 9 & occ;

		while (left) {
			dst = pop_lsb(left);
			push(makeMove(dst - L, dst));
		}

		while (right) {
			dst = pop_lsb(right);
			push(makeMove(dst - R, dst));
		}
	}

	if (T != MoveType::Attacks) {
		U64 empty = mState.getEmptyBB() & ~RANK_PROMOTION;

		U64 up  = (C == WHITE ? pawns << 8 : pawns >> 8) & empty;
		U64 dbl = (C == WHITE ? (up & RANK_3) << 8
			: (up & RANK_6) >> 8) & empty;

		if (T == MoveType::Evasions) {
			up  &= mValid;
			dbl &= mValid;
		}

		if (T == MoveType::QuietChecks) {
			U64 dis;

			up  &= mState.getCheckSquaresBB(PIECETYPE_PAWN);
			dbl &= mState.getCheckSquaresBB(PIECETYPE_PAWN);

			dis = mDiscover & pawns & ~file_bb[mState.getKingSquare(!C)];
			dis = (C == WHITE ? dis << 8 : dis >> 8) & empty;
			up  |= dis;
			dbl |= (C == WHITE ? (dis & RANK_3) << 8 
				: (dis & RANK_6) >> 8) & empty;
		}

		while (up) {
			dst = pop_lsb(up);
			push(makeMove(dst - U, dst));
		}
		while (dbl) {
			dst = pop_lsb(dbl);
			push(makeMove(dst - U - U, dst));
		}
	}
}

template<MoveType T> void MoveList::pushCastle() {
	Square k = mState.getKingSquare(mState.getOurColor());
	if (mState.canCastleKingside() && !(between_hor[k][k - 3] & mState.getOccupancyBB()) && !mState.attacked(k - 1) && !mState.attacked(k - 2)) {
		if (T == MoveType::QuietChecks) {
			if (square_bb[k - 1] & mState.getCheckSquaresBB(PIECETYPE_ROOK)) {
				push(makeCastle(k, k - 2));
			}
		}
		else {
			push(makeCastle(k, k - 2));
		}
	}
	if (mState.canCastleQueenside() && !(between_hor[k][k + 4] & mState.getOccupancyBB()) && !mState.attacked(k + 1) && !mState.attacked(k + 2)) {
		if (T == MoveType::QuietChecks) {
			if (square_bb[k + 1] & mState.getCheckSquaresBB(PIECETYPE_ROOK)) {
				push(makeCastle(k, k + 2));
			}
		}
		else {
			push(makeCastle(k, k + 2));
		}
	}
}

void MoveList::checkLegal() {
	for (int i = 0; i < mSize; ++i) {
		if (!mState.isLegal(mList[i].move)) {
			mList[i--] = mList[--mSize];
		}
	}
}

template<MoveType T> void MoveList::generateMoves() {
	assert(!mState.inCheck());

	if (T == MoveType::QuietChecks) {
		mDiscover = mState.getDiscoveredChecks(mState.getOurColor());
	}
	mValid = T == MoveType::Attacks ? mState.getOccupancyBB(mState.getTheirColor()) : mState.getEmptyBB();

	mState.getOurColor() == WHITE ? pushPawnMoves<T, WHITE>() : pushPawnMoves<T, BLACK>();
	pushMoves<T, PIECETYPE_KNIGHT>();
	pushMoves<T, PIECETYPE_BISHOP>();
	pushMoves<T, PIECETYPE_ROOK>();
	pushMoves<T, PIECETYPE_QUEEN>();
	pushMoves<T, PIECETYPE_KING>();
}

template<> void MoveList::generateMoves<MoveType::Evasions>() {
	assert(mState.inCheck());

	Square k, c;

	mValid = mState.getOccupancyBB(mState.getTheirColor()) | mState.getEmptyBB();
	pushMoves<MoveType::Evasions, PIECETYPE_KING>();

	if (mState.inDoubleCheck()) {
		return;
	}

	k = mState.getKingSquare(mState.getOurColor());
	c = get_lsb(mState.getCheckersBB());
	mValid = between[k][c] | mState.getCheckersBB();

	mState.getOurColor() == WHITE ? pushPawnMoves<MoveType::Evasions, WHITE>() : pushPawnMoves<MoveType::Evasions, BLACK>();
	pushMoves<MoveType::Evasions, PIECETYPE_KNIGHT>();
	pushMoves<MoveType::Evasions, PIECETYPE_BISHOP>();
	pushMoves<MoveType::Evasions, PIECETYPE_ROOK>();
	pushMoves<MoveType::Evasions, PIECETYPE_QUEEN>();
}

template<> void MoveList::generateMoves<MoveType::All>() {
	if (mState.inCheck()) {
		generateMoves<MoveType::Evasions>();
		return;
	}

	mValid = mState.getOccupancyBB(mState.getTheirColor()) | mState.getEmptyBB();

	mState.getOurColor() == WHITE ? pushPawnMoves<MoveType::All, WHITE>() : pushPawnMoves<MoveType::All, BLACK>();
	pushMoves<MoveType::All, PIECETYPE_KNIGHT>();
	pushMoves<MoveType::All, PIECETYPE_BISHOP>();
	pushMoves<MoveType::All, PIECETYPE_ROOK>();
	pushMoves<MoveType::All, PIECETYPE_QUEEN>();
	pushMoves<MoveType::All, PIECETYPE_KING>();
}

/* Get the best move from the MoveList based on the generation stage */
Move MoveList::getBestMove() {
	Move move;
	
	switch (mStage) {
		case BestMove:
			// Best move
			// If the move given by the pv or tt is valid, return it.
			mStage++;
			if (mState.isValid(mBest, FULL) && mState.isLegal(mBest)) {
				return mBest;
			}
		case AttacksGen:
			// Capture generation
			// Generate and sort capture moves in the event that Best move failed.
			// If a move has a positive SEE, score it by LVA-MVV, otherwise store 
			// its score as its SEE value.
			generateMoves<MoveType::Attacks>();
			for (int i = 0; i < mSize; ++i) {
				int see = mState.see(mList[i].move);
				if (see >= 0) {
					mList[i].score = mState.onSquare(getDst(mList[i].move)) - mState.onSquare(getSrc(mList[i].move));
				}
				else {
					mList[i].score = see;
					badCaptures.push_back(mList[i]);
					std::swap(mList[i], mList[mSize - 1]);
					pop();
				}
			}
			mStage++;

		case Attacks:
			// Captures
			// No need to sort the captures since there's not many of them in any 
			// given position. Ensure that each capture move has not already been
			// seen before!
			while (mSize) {
				std::iter_swap(std::max_element(mList.begin(), mList.begin() + mSize), mList.begin() + mSize - 1);
				move = pop();
				if (move != mBest && mState.isLegal(move)) {
					return move;
				}
			}
			mStage++;

		case Killer1:
			// First killer move
			// Ensure that the killer stored at this ply is a valid move and is not 
			// the same as the best move. Return it before generating any quiets.
			mStage++;
			if ((mState.isValid(mKiller1, FULL) && mState.isLegal(mKiller1) && mState.isQuiet(mKiller1)) && mKiller1 != mBest) {
				return mKiller1;
			}

		case Killer2:
			// Second killer move
			// Ensure that the killer stored at this ply is a valid move and is not 
			// the same as the best move or the first killer move. Return it before 
			// generating any quiets.
			mStage++;
			if ((mState.isValid(mKiller2, FULL)) && mState.isLegal(mKiller2) && mState.isQuiet(mKiller2) && mKiller2 != mBest && mKiller2 != mKiller1) {
				return mKiller2;
			}
		
		case QuietsGen:
			// Quiet generation
			// Generate and order quiet moves, sort them first by their history scores.
			// Moves without a history score will be partitioned and sorted to the 
			// right, so that they are checked first. All other quiets will be sorted
			// based on their PST score.
			{
			generateMoves<MoveType::Quiets>();
			for (int i = 0; i < mSize; ++i) {
				mList[i].score = mHistory->getHistoryScore(mList[i].move);
			}
			auto it2 = std::partition(mList.begin(), mList.begin() + mSize, noScore);
			std::stable_sort(it2, mList.begin() + mSize);
			auto it1 = mList.begin();
			for (auto it1 = mList.begin(); it1 != it2; ++it1) {
				Square src = getSrc(it1->move);
				Square dst = getDst(it1->move);
				PieceType toMove = mState.onSquare(src);
				it1->score = PieceSquareTable::getTaperedScore(toMove, mState.getOurColor(), dst, mState.getGamePhase()) 
					- PieceSquareTable::getTaperedScore(toMove, mState.getOurColor(), src, mState.getGamePhase());
			}
			std::stable_sort(mList.begin(), it2);
			mStage++;
			}
		
		case Quiets:
			// Quiet moves
			// Ensure that each move is different from the best move and killers.
			while (mSize) {
				move = pop();
				if (move != mBest && move != mKiller1 && move != mKiller2 && mState.isLegal(move)) {
					return move;
				}
			}
			while (!badCaptures.empty()) {
				move = badCaptures.back().move;
				badCaptures.pop_back();
				if (move != mBest && mState.isLegal(move)) {
					return move;
				}
			}
			break;

		case QBestMove:
			// Qsearch best move
			// If the move given by the pv or tt is valid, return it.
			mStage++;
			if (mState.isValid(mBest, FULL) && mState.isLegal(mBest)) {
				return mBest;
			}

		case QAttacksGen:
			// Qsearch capture generation
			// Generate and sort captures based on LVA-MVV.
			generateMoves<MoveType::Attacks>();
			for (int i = 0; i < mSize; ++i) {
				mList[i].score = mState.onSquare(getDst(mList[i].move)) - mState.onSquare(getSrc(mList[i].move));
			}
			mStage++;

		case QAttacks:
			// Qsearch captures
			// Ensure that each capture move is different from the best move.
			while (mSize) {
				std::iter_swap(std::max_element(mList.begin(), mList.begin() + mSize), mList.begin() + mSize - 1);
				move = pop();
				if (mState.see(move) < 0 && !isPromotion(move)) {
					continue;
				}
				if (move != mBest && mState.isLegal(move)) {
					return move;
				}
			}
			mStage++;

		case QQuietChecksGen:
			// Qsearch quiet generation
			// Generate quiet moves.
			for (int i = 0; i < mSize; ++i) {
				mList[i].score = mHistory->getHistoryScore(mList[i].move);
			}
			mStage++;

		case QQuietChecks:
			// Qsearch quiet checks
			// Ensure that each move is different from the best move.
			while (mSize) {
				std::iter_swap(std::max_element(mList.begin(), mList.begin() + mSize), mList.begin() + mSize - 1);
				move = pop();
				if (move != mBest && mState.isLegal(move)) {
					return move;
				}
			}
			break;

		case EvadeBestMove:
			// Evade best move
			// If the move given by the pv or tt is valid, return it.
			mStage++;
			if (mState.isValid(mBest, mValid) && mState.isLegal(mBest) && !mState.inDoubleCheck()) {
				return mBest;
			}

		case EvadeMovesGen:
			// Evade move generation
			// Generate King captures.
			{
			const int CaptureFlag = 0x40000000;
			const int HistoryFlag = 0x20000000;
			generateMoves<MoveType::Evasions>();
			for (int i = 0; i < mSize; ++i) {
				if (mState.isCapture(mList[i].move)) {
					int see = mState.see(mList[i].move);
					if (see >= 0) {
						mList[i].score = mState.onSquare(getDst(mList[i].move)) - mState.onSquare(getSrc(mList[i].move));
					}
					else {
						mList[i].score = see;
					}
					mList[i].score |= CaptureFlag;
				}
				else {
					int hist = mHistory->getHistoryScore(mList[i].move);
					if (hist > 0) {
						mList[i].score = hist | HistoryFlag;
					}
					else {
						Square src = getSrc(mList[i].move);
						Square dst = getDst(mList[i].move);
						PieceType piece = mState.onSquare(src);
						mList[i].score = PieceSquareTable::getTaperedScore(piece, mState.getOurColor(), dst, mState.getGamePhase()) 
										- PieceSquareTable::getTaperedScore(piece, mState.getOurColor(), src, mState.getGamePhase());
					}
				}
			}
			std::stable_sort(mList.begin(), mList.begin() + mSize);
			mStage++;
			}
	
		case Evade:
			// Evade
			// Return the capture move if it's not the same as the best move.
			while (mSize) {
				move = pop();
				if (move != mBest && mState.isLegal(move)) {
					return move;
				}
			}
			break;

		case AllLegal:
			// All legal
			// Generate and return each legal move.
			while (mSize) {
				move = pop();
				return move;
			}
			break;
		
		default:
			// ERROR!
			assert(false);
	}
	
	return NULL_MOVE; // No move found?!?
}

/* List of moves and related functions */
MoveList::MoveList(const State& s) : mState(s), mValid(FULL), mQSearch(false), mBest(NULL_MOVE), mSize(0), mHistory(nullptr), mPly(0) {
	generateMoves<MoveType::All>();
	mStage = AllLegal;
	checkLegal();
}

std::ostream& operator<<(std::ostream& os, const MoveList& moveList) {
	MoveList ml = moveList;
	os << ml.size() << " legal moves:\n";
	while (ml.size() >= 1) {
		Move move = ml.pop();
		os << to_string(move) << "\n";
	}
	return os;
}