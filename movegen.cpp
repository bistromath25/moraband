#include "movegen.h"

void mg_init() {
	initmagicmoves();
}

MoveList::MoveList(const State& pState, Move pBest, History* pHistory, int pPly, bool pQSearch) 
: mState(pState), mValid(Full), mBest(pBest), mQSearch(pQSearch), mKiller1(NULL_MOVE), mKiller2(NULL_MOVE) , mSize(0), mHistory(pHistory), mPly(pPly) {
	if (pHistory) {
		mKiller1 = mHistory->getKiller(pPly).first;
		mKiller2 = mHistory->getKiller(pPly).second;
	}
	
	if (mState.inCheck()) {
		Square k = mState.getKingSquare(mState.getOurColor());
		Square c = get_lsb(mState.getCheckersBB());
		mValid = between[k][c] | mState.getCheckersBB();
		mStage = nEvadeBestMove;
	}
	else {
		mStage = mQSearch ? qBestMove : nBestMove;
	}
}

std::size_t MoveList::size() const {
	return mSize;
}

void MoveList::push(Move pMove) {
	mList[mSize++].move = pMove;
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
			push(makeMove(src, dst + 1, queen));
		}
		if (T == MoveType::Quiets || T == MoveType::All) {
			push(makeMove(src, dst + 1, knight));
			push(makeMove(src, dst + 1, rook));
			push(makeMove(src, dst + 1, bishop));
		}
		if (T == MoveType::Evasions) {
			if (square_bb[dst + 1] & mValid) {
				push(makeMove(src, dst + 1, queen));
				push(makeMove(src, dst + 1, knight));
				push(makeMove(src, dst + 1, rook));
				push(makeMove(src, dst + 1, bishop));
			}
		}
		if (T == MoveType::QuietChecks)	{
			if (mState.getCheckSquaresBB(knight) & square_bb[dst + 1]) {
				push(makeMove(src, dst + 1, knight));
			}
		}
	}

	if (square_bb[dst] & NOT_H_FILE && square_bb[dst - 1] & mState.getOccupancyBB(!C)) {
		if (T == MoveType::Attacks || T == MoveType::All) {
			push(makeMove(src, dst - 1, queen));
		}
		if (T == MoveType::Quiets || T == MoveType::All) {
			push(makeMove(src, dst - 1, knight));
			push(makeMove(src, dst - 1, rook));
			push(makeMove(src, dst - 1, bishop));
		}
		if (T == MoveType::Evasions) {
			if (square_bb[dst - 1] & mValid) {
				push(makeMove(src, dst - 1, queen));
				push(makeMove(src, dst - 1, knight));
				push(makeMove(src, dst - 1, rook));
				push(makeMove(src, dst - 1, bishop));
			}
		}
		if (T == MoveType::QuietChecks) {
			if (mState.getCheckSquaresBB(knight) & square_bb[dst - 1]) {
				push(makeMove(src, dst - 1, knight));
			}
		}
	}

	if (square_bb[dst] & mState.getEmptyBB()) {
		if (T == MoveType::Attacks || T == MoveType::All) {
			push(makeMove(src, dst, queen));
		}
		if (T == MoveType::Quiets || T == MoveType::All) {
			push(makeMove(src, dst, knight));
			push(makeMove(src, dst, rook));
			push(makeMove(src, dst, bishop));
		}
		if (T == MoveType::Evasions) {
			if (square_bb[dst] & mValid) {
				push(makeMove(src, dst, queen));
				push(makeMove(src, dst, knight));
				push(makeMove(src, dst, rook));
				push(makeMove(src, dst, bishop));
			}
		}
		if (T == MoveType::QuietChecks) {
			if (mState.getCheckSquaresBB(knight) & square_bb[dst]) {
				push(makeMove(src, dst, knight));
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
				if (P == king) {
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

		if (P == king && T != MoveType::Attacks && T != MoveType::Evasions) {
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
	pawns = mState.getPieceBB<pawn>(C);
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

			up  &= mState.getCheckSquaresBB(pawn);
			dbl &= mState.getCheckSquaresBB(pawn);

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
	if (mState.canCastleKingside() && !(between_hor[k][k-3] & mState.getOccupancyBB()) && !mState.attacked(k - 1) && !mState.attacked(k-2)) {
		if (T == MoveType::QuietChecks) {
			if (square_bb[k - 1] & mState.getCheckSquaresBB(rook)) {
				push(makeCastle(k, k-2));
			}
		}
		else {
			push(makeCastle(k, k-2));
		}
	}
	if (mState.canCastleQueenside() && !(between_hor[k][k+4] & mState.getOccupancyBB()) && !mState.attacked(k + 1) && !mState.attacked(k+2)) {
		if (T == MoveType::QuietChecks) {
			if (square_bb[k + 1] & mState.getCheckSquaresBB(rook)) {
				push(makeCastle(k, k+2));
			}
		}
		else {
			push(makeCastle(k, k+2));
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
	pushMoves<T, knight>();
	pushMoves<T, bishop>();
	pushMoves<T, rook>();
	pushMoves<T, queen>();
	pushMoves<T, king>();
}

template<> void MoveList::generateMoves<MoveType::Evasions>() {
	assert(mState.inCheck());

	Square k, c;

	mValid = mState.getOccupancyBB(mState.getTheirColor()) | mState.getEmptyBB();
	pushMoves<MoveType::Evasions, king>();

	if (mState.inDoubleCheck()) {
		return;
	}

	k = mState.getKingSquare(mState.getOurColor());
	c = get_lsb(mState.getCheckersBB());
	mValid = between[k][c] | mState.getCheckersBB();

	mState.getOurColor() == WHITE ? pushPawnMoves<MoveType::Evasions, WHITE>() : pushPawnMoves<MoveType::Evasions, BLACK>();
	pushMoves<MoveType::Evasions, knight>();
	pushMoves<MoveType::Evasions, bishop>();
	pushMoves<MoveType::Evasions, rook>();
	pushMoves<MoveType::Evasions, queen>();
}

template<> void MoveList::generateMoves<MoveType::All>() {
	if (mState.inCheck()) {
		generateMoves<MoveType::Evasions>();
		return;
	}

	mValid = mState.getOccupancyBB(mState.getTheirColor()) | mState.getEmptyBB();

	mState.getOurColor() == WHITE ? pushPawnMoves<MoveType::All, WHITE>() : pushPawnMoves<MoveType::All, BLACK>();
	pushMoves<MoveType::All, knight>();
	pushMoves<MoveType::All, bishop>();
	pushMoves<MoveType::All, rook>();
	pushMoves<MoveType::All, queen>();
	pushMoves<MoveType::All, king>();
}

Move MoveList::getBestMove() {
	Move move;
	
	switch (mStage) {
		case nBestMove:
			mStage++;
			if (mState.isValid(mBest, Full) && mState.isLegal(mBest)) {
				return mBest;
			}
		case nAttacksGen:
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

		case nAttacks:
			while (mSize) {
				std::iter_swap(std::max_element(mList.begin(), mList.begin() + mSize), mList.begin() + mSize - 1);
				move = pop();
				if (move != mBest && mState.isLegal(move)) {
					return move;
				}
			}
			mStage++;

		case nKiller1:
			mStage++;
			if ((mState.isValid(mKiller1, Full) && mState.isLegal(mKiller1) && mState.isQuiet(mKiller1)) && mKiller1 != mBest) {
				return mKiller1;
			}

		case nKiller2:
			mStage++;
			if ((mState.isValid(mKiller2, Full)) && mState.isLegal(mKiller2) && mState.isQuiet(mKiller2) && mKiller2 != mBest && mKiller2 != mKiller1) {
				return mKiller2;
			}
		
		case nQuietsGen:
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
				it1->score = PieceSquareTable::getTaperedScore(mState.getGamePhase(), 
				toMove, mState.getOurColor(), dst) -
			PieceSquareTable::getTaperedScore(mState.getGamePhase(), toMove, mState.getOurColor(), src);
			}
			std::stable_sort(mList.begin(), it2);
			mStage++;
			}
		
		case nQuiets:
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

		case qBestMove:
			mStage++;
			if (mState.isValid(mBest, Full) && mState.isLegal(mBest)) {
				return mBest;
			}

		case qAttacksGen:
			generateMoves<MoveType::Attacks>();
			for (int i = 0; i < mSize; ++i) {
				mList[i].score = mState.onSquare(getDst(mList[i].move)) - mState.onSquare(getSrc(mList[i].move));
			}
			mStage++;

		case qAttacks:
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

		case qQuietChecksGen:
			for (int i = 0; i < mSize; ++i) {
				mList[i].score = mHistory->getHistoryScore(mList[i].move);
			}
			mStage++;

		case qQuietChecks:
			while (mSize) {
				std::iter_swap(std::max_element(mList.begin(), mList.begin() + mSize), mList.begin() + mSize - 1);
				move = pop();
				if (move != mBest && mState.isLegal(move)) {
					return move;
				}
			}
			break;

		case nEvadeBestMove:
			mStage++;
			if (mState.isValid(mBest, mValid) && mState.isLegal(mBest) && !mState.inDoubleCheck()) {
				return mBest;
			}

		case nEvadeMovesGen:
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
						mList[i].score = PieceSquareTable::getTaperedScore(mState.getGamePhase(), piece, mState.getOurColor(), dst) - PieceSquareTable::getTaperedScore(mState.getGamePhase(), piece, mState.getOurColor(), src);
					}
				}
			}
			std::stable_sort(mList.begin(), mList.begin() + mSize);
			mStage++;
			}
	
		case nEvade:
			while (mSize) {
				move = pop();
				if (move != mBest && mState.isLegal(move)) {
					return move;
				}
			}
		break;

		case allLegal:
			while (mSize) {
				move = pop();
				return move;
			}
			break;
		
		default:
			assert(false);
	}
	
	return NULL_MOVE;
}

Move MoveList::getKiller1() const {
	return mKiller1;
}

Move MoveList::getKiller2() const {
	return mKiller2;
}

void MoveList::setKiller1(Move m) {
	mKiller1 = m;
}

void MoveList::setKiller2(Move m) {
	mKiller2 = m;
}

MoveList::MoveList(const State& pState) : mState(pState), mValid(Full), mQSearch(false), mBest(NULL_MOVE), mSize(0), mHistory(nullptr), mPly(0) {
	generateMoves<MoveType::All>();
	mStage = allLegal;
	checkLegal();
}

///