/**
 * Moraband, known in antiquity as Korriban, was an 
 * Outer Rim planet that was home to the ancient Sith 
 **/

#include "movegen.h"

/* Initialize magic moves */
void mg_init() {
	initmagicmoves();
}

std::size_t MoveList::size() const {
	return sz;
}

void MoveList::push(Move move) {
	moveList[sz++].move = move;
}

Move MoveList::pop() {
	return moveList[--sz].move;
}

bool MoveList::contains(Move move) const {
	return std::find(moveList.begin(), moveList.begin() + sz, move) != moveList.begin() + sz;
}

template<MoveType T> void MoveList::pushPromotion(Square src, Square dst) {
	const Color c = position.getOurColor();
	if (square_bb[dst] & NOT_A_FILE && square_bb[dst + 1] & position.getOccupancyBB(!c)) {
		if (T == MoveType::Attacks || T == MoveType::All) {
			push(makeMove(src, dst + 1, PIECETYPE_QUEEN));
		}
		if (T == MoveType::Quiets || T == MoveType::All) {
			push(makeMove(src, dst + 1, PIECETYPE_KNIGHT));
			push(makeMove(src, dst + 1, PIECETYPE_ROOK));
			push(makeMove(src, dst + 1, PIECETYPE_BISHOP));
		}
		if (T == MoveType::Evasions) {
			if (square_bb[dst + 1] & valid) {
				push(makeMove(src, dst + 1, PIECETYPE_QUEEN));
				push(makeMove(src, dst + 1, PIECETYPE_KNIGHT));
				push(makeMove(src, dst + 1, PIECETYPE_ROOK));
				push(makeMove(src, dst + 1, PIECETYPE_BISHOP));
			}
		}
		if (T == MoveType::QuietChecks)	{
			if (position.getCheckSquaresBB(PIECETYPE_KNIGHT) & square_bb[dst + 1]) {
				push(makeMove(src, dst + 1, PIECETYPE_KNIGHT));
			}
		}
	}

	if (square_bb[dst] & NOT_H_FILE && square_bb[dst - 1] & position.getOccupancyBB(!c)) {
		if (T == MoveType::Attacks || T == MoveType::All) {
			push(makeMove(src, dst - 1, PIECETYPE_QUEEN));
		}
		if (T == MoveType::Quiets || T == MoveType::All) {
			push(makeMove(src, dst - 1, PIECETYPE_KNIGHT));
			push(makeMove(src, dst - 1, PIECETYPE_ROOK));
			push(makeMove(src, dst - 1, PIECETYPE_BISHOP));
		}
		if (T == MoveType::Evasions) {
			if (square_bb[dst - 1] & valid) {
				push(makeMove(src, dst - 1, PIECETYPE_QUEEN));
				push(makeMove(src, dst - 1, PIECETYPE_KNIGHT));
				push(makeMove(src, dst - 1, PIECETYPE_ROOK));
				push(makeMove(src, dst - 1, PIECETYPE_BISHOP));
			}
		}
		if (T == MoveType::QuietChecks) {
			if (position.getCheckSquaresBB(PIECETYPE_KNIGHT) & square_bb[dst - 1]) {
				push(makeMove(src, dst - 1, PIECETYPE_KNIGHT));
			}
		}
	}

	if (square_bb[dst] & position.getEmptyBB()) {
		if (T == MoveType::Attacks || T == MoveType::All) {
			push(makeMove(src, dst, PIECETYPE_QUEEN));
		}
		if (T == MoveType::Quiets || T == MoveType::All) {
			push(makeMove(src, dst, PIECETYPE_KNIGHT));
			push(makeMove(src, dst, PIECETYPE_ROOK));
			push(makeMove(src, dst, PIECETYPE_BISHOP));
		}
		if (T == MoveType::Evasions) {
			if (square_bb[dst] & valid) {
				push(makeMove(src, dst, PIECETYPE_QUEEN));
				push(makeMove(src, dst, PIECETYPE_KNIGHT));
				push(makeMove(src, dst, PIECETYPE_ROOK));
				push(makeMove(src, dst, PIECETYPE_BISHOP));
			}
		}
		if (T == MoveType::QuietChecks) {
			if (position.getCheckSquaresBB(PIECETYPE_KNIGHT) & square_bb[dst]) {
				push(makeMove(src, dst, PIECETYPE_KNIGHT));
			}
		}
	}
}

template<MoveType T, PieceType P> void MoveList::pushMoves() {
	U64 m;
	Color c = position.getOurColor();

	for (Square src : position.getPieceList<P>(c)) {
		if (src == no_sq) {
			break;
		}
		m = position.getAttackBB<P>(src) & valid;

		if (T == MoveType::QuietChecks) {
			if (square_bb[src] & discover) {
				if (P == PIECETYPE_KING) {
					m &= ~coplanar[src][position.getKingSquare(!c)];
				}
			}
			else {
				m &= position.getCheckSquaresBB(P);
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
	pawns = position.getPieceBB<PIECETYPE_PAWN>(C);
	promo = (C == WHITE ? pawns << 8 : pawns >> 8) & RANK_PROMOTION;

	while (promo) {
		dst = pop_lsb(promo);
		pushPromotion<T>(dst - U, dst);
	}

	if (T == MoveType::Attacks || T == MoveType::Evasions || T == MoveType::All) {
		U64 occ = ((T == MoveType::Evasions ? position.getOccupancyBB(!C) & valid
			: position.getOccupancyBB(!C))
				| position.getEnPassantBB()) & ~RANK_PROMOTION;

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
		U64 empty = position.getEmptyBB() & ~RANK_PROMOTION;

		U64 up  = (C == WHITE ? pawns << 8 : pawns >> 8) & empty;
		U64 dbl = (C == WHITE ? (up & RANK_3) << 8
			: (up & RANK_6) >> 8) & empty;

		if (T == MoveType::Evasions) {
			up  &= valid;
			dbl &= valid;
		}

		if (T == MoveType::QuietChecks) {
			U64 dis;

			up  &= position.getCheckSquaresBB(PIECETYPE_PAWN);
			dbl &= position.getCheckSquaresBB(PIECETYPE_PAWN);

			dis = discover & pawns & ~file_bb[position.getKingSquare(!C)];
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
	Square k = position.getKingSquare(position.getOurColor());
	if (position.canCastleKingside() && !(between_hor[k][k - 3] & position.getOccupancyBB()) && !position.attacked(k - 1) && !position.attacked(k - 2)) {
		if (T == MoveType::QuietChecks) {
			if (square_bb[k - 1] & position.getCheckSquaresBB(PIECETYPE_ROOK)) {
				push(makeCastle(k, k - 2));
			}
		}
		else {
			push(makeCastle(k, k - 2));
		}
	}
	if (position.canCastleQueenside() && !(between_hor[k][k + 4] & position.getOccupancyBB()) && !position.attacked(k + 1) && !position.attacked(k + 2)) {
		if (T == MoveType::QuietChecks) {
			if (square_bb[k + 1] & position.getCheckSquaresBB(PIECETYPE_ROOK)) {
				push(makeCastle(k, k + 2));
			}
		}
		else {
			push(makeCastle(k, k + 2));
		}
	}
}

void MoveList::checkLegal() {
	for (int i = 0; i < int(sz); ++i) {
		if (!position.isLegal(moveList[i].move)) {
			moveList[i--] = moveList[--sz];
		}
	}
}

template<MoveType T> void MoveList::generateMoves() {
	assert(!position.inCheck());

	if (T == MoveType::QuietChecks) {
		discover = position.getDiscoveredChecks(position.getOurColor());
	}
	valid = T == MoveType::Attacks ? position.getOccupancyBB(position.getTheirColor()) : position.getEmptyBB();

	position.getOurColor() == WHITE ? pushPawnMoves<T, WHITE>() : pushPawnMoves<T, BLACK>();
	pushMoves<T, PIECETYPE_KNIGHT>();
	pushMoves<T, PIECETYPE_BISHOP>();
	pushMoves<T, PIECETYPE_ROOK>();
	pushMoves<T, PIECETYPE_QUEEN>();
	pushMoves<T, PIECETYPE_KING>();
}

template<> void MoveList::generateMoves<MoveType::Evasions>() {
	assert(position.inCheck());

	Square k, c;

	valid = position.getOccupancyBB(position.getTheirColor()) | position.getEmptyBB();
	pushMoves<MoveType::Evasions, PIECETYPE_KING>();

	if (position.inDoubleCheck()) {
		return;
	}

	k = position.getKingSquare(position.getOurColor());
	c = get_lsb(position.getCheckersBB());
	valid = between[k][c] | position.getCheckersBB();

	position.getOurColor() == WHITE ? pushPawnMoves<MoveType::Evasions, WHITE>() : pushPawnMoves<MoveType::Evasions, BLACK>();
	pushMoves<MoveType::Evasions, PIECETYPE_KNIGHT>();
	pushMoves<MoveType::Evasions, PIECETYPE_BISHOP>();
	pushMoves<MoveType::Evasions, PIECETYPE_ROOK>();
	pushMoves<MoveType::Evasions, PIECETYPE_QUEEN>();
}

template<> void MoveList::generateMoves<MoveType::All>() {
	if (position.inCheck()) {
		generateMoves<MoveType::Evasions>();
		return;
	}

	valid = position.getOccupancyBB(position.getTheirColor()) | position.getEmptyBB();

	position.getOurColor() == WHITE ? pushPawnMoves<MoveType::All, WHITE>() : pushPawnMoves<MoveType::All, BLACK>();
	pushMoves<MoveType::All, PIECETYPE_KNIGHT>();
	pushMoves<MoveType::All, PIECETYPE_BISHOP>();
	pushMoves<MoveType::All, PIECETYPE_ROOK>();
	pushMoves<MoveType::All, PIECETYPE_QUEEN>();
	pushMoves<MoveType::All, PIECETYPE_KING>();
}

/* Get the best move from the MoveList based on the generation stage */
Move MoveList::getBestMove() {
	Move move;
	
	switch (stage) {
		case BestMove:
			// Best move
			// If the move given by the pv or tt is valid, return it.
			stage++;
			if (position.isValid(best, FULL) && position.isLegal(best)) {
				return best;
			}
		case AttacksGen:
			// Capture generation
			// Generate and sort capture moves in the event that Best move failed.
			// If a move has a positive SEE, score it by LVA-MVV, otherwise store 
			// its score as its SEE value.
			generateMoves<MoveType::Attacks>();
			for (int i = 0; i < int(sz); ++i) {
				int see = position.see(moveList[i].move);
				if (see >= 0) {
					moveList[i].score = position.onSquare(getDst(moveList[i].move)) - position.onSquare(getSrc(moveList[i].move));
				}
				else {
					moveList[i].score = see;
					badCaptures.push_back(moveList[i]);
					std::swap(moveList[i], moveList[sz - 1]);
					pop();
				}
			}
			stage++;

		case Attacks:
			// Captures
			// No need to sort the captures since there's not many of them in any 
			// given position. Ensure that each capture move has not already been
			// seen before!
			while (sz) {
				std::iter_swap(std::max_element(moveList.begin(), moveList.begin() + sz), moveList.begin() + sz - 1);
				move = pop();
				if (move != best && position.isLegal(move)) {
					return move;
				}
			}
			stage++;

		case Killer1:
			// First killer move
			// Ensure that the killer stored at this ply is a valid move and is not 
			// the same as the best move. Return it before generating any quiets.
			stage++;
			if ((position.isValid(killer1, FULL) && position.isLegal(killer1) && position.isQuiet(killer1)) && killer1 != best) {
				return killer1;
			}

		case Killer2:
			// Second killer move
			// Ensure that the killer stored at this ply is a valid move and is not 
			// the same as the best move or the first killer move. Return it before 
			// generating any quiets.
			stage++;
			if ((position.isValid(killer2, FULL)) && position.isLegal(killer2) && position.isQuiet(killer2) && killer2 != best && killer2 != killer1) {
				return killer2;
			}
		
		case QuietsGen:
			// Quiet generation
			// Generate and order quiet moves, sort them first by their history scores.
			// Moves without a history score will be partitioned and sorted to the 
			// right, so that they are checked first. All other quiets will be sorted
			// based on their PST score.
			{
			generateMoves<MoveType::Quiets>();
			for (int i = 0; i < int(sz); ++i) {
				moveList[i].score = history->getHistoryScore(moveList[i].move);
			}
			auto it2 = std::partition(moveList.begin(), moveList.begin() + sz, noScore);
			std::stable_sort(it2, moveList.begin() + sz);
			for (auto it1 = moveList.begin(); it1 != it2; ++it1) {
				Square src = getSrc(it1->move);
				Square dst = getDst(it1->move);
				PieceType toMove = position.onSquare(src);
				it1->score = PieceSquareTable::getTaperedScore(toMove, position.getOurColor(), dst, position.getGamePhase()) 
					- PieceSquareTable::getTaperedScore(toMove, position.getOurColor(), src, position.getGamePhase());
			}
			std::stable_sort(moveList.begin(), it2);
			stage++;
			}
		
		case Quiets:
			// Quiet moves
			// Ensure that each move is different from the best move and killers.
			while (sz) {
				move = pop();
				if (move != best && move != killer1 && move != killer2 && position.isLegal(move)) {
					return move;
				}
			}
			while (!badCaptures.empty()) {
				move = badCaptures.back().move;
				badCaptures.pop_back();
				if (move != best && position.isLegal(move)) {
					return move;
				}
			}
			break;

		case QBestMove:
			// Qsearch best move
			// If the move given by the pv or tt is valid, return it.
			stage++;
			if (position.isValid(best, FULL) && position.isLegal(best)) {
				return best;
			}

		case QAttacksGen:
			// Qsearch capture generation
			// Generate and sort captures based on LVA-MVV.
			generateMoves<MoveType::Attacks>();
			for (int i = 0; i < int(sz); ++i) {
				moveList[i].score = position.onSquare(getDst(moveList[i].move)) - position.onSquare(getSrc(moveList[i].move));
			}
			stage++;

		case QAttacks:
			// Qsearch captures
			// Ensure that each capture move is different from the best move.
			while (sz) {
				std::iter_swap(std::max_element(moveList.begin(), moveList.begin() + sz), moveList.begin() + sz - 1);
				move = pop();
				if (position.see(move) < 0 && !isPromotion(move)) {
					continue;
				}
				if (move != best && position.isLegal(move)) {
					return move;
				}
			}
			stage++;

		case QQuietChecksGen:
			// Qsearch quiet generation
			// Generate quiet moves.
			for (int i = 0; i < int(sz); ++i) {
				moveList[i].score = history->getHistoryScore(moveList[i].move);
			}
			stage++;

		case QQuietChecks:
			// Qsearch quiet checks
			// Ensure that each move is different from the best move.
			while (sz) {
				std::iter_swap(std::max_element(moveList.begin(), moveList.begin() + sz), moveList.begin() + sz - 1);
				move = pop();
				if (move != best && position.isLegal(move)) {
					return move;
				}
			}
			break;

		case EvadeBestMove:
			// Evade best move
			// If the move given by the pv or tt is valid, return it.
			stage++;
			if (position.isValid(best, valid) && position.isLegal(best) && !position.inDoubleCheck()) {
				return best;
			}

		case EvadeMovesGen:
			// Evade move generation
			// Generate King captures.
			{
			const int CaptureFlag = 0x40000000;
			const int HistoryFlag = 0x20000000;
			generateMoves<MoveType::Evasions>();
			for (int i = 0; i < int(sz); ++i) {
				if (position.isCapture(moveList[i].move)) {
					int see = position.see(moveList[i].move);
					if (see >= 0) {
						moveList[i].score = position.onSquare(getDst(moveList[i].move)) - position.onSquare(getSrc(moveList[i].move));
					}
					else {
						moveList[i].score = see;
					}
					moveList[i].score |= CaptureFlag;
				}
				else {
					int hist = history->getHistoryScore(moveList[i].move);
					if (hist > 0) {
						moveList[i].score = hist | HistoryFlag;
					}
					else {
						Square src = getSrc(moveList[i].move);
						Square dst = getDst(moveList[i].move);
						PieceType piece = position.onSquare(src);
						moveList[i].score = PieceSquareTable::getTaperedScore(piece, position.getOurColor(), dst, position.getGamePhase()) 
										- PieceSquareTable::getTaperedScore(piece, position.getOurColor(), src, position.getGamePhase());
					}
				}
			}
			std::stable_sort(moveList.begin(), moveList.begin() + sz);
			stage++;
			}
	
		case Evade:
			// Evade
			// Return the capture move if it's not the same as the best move.
			while (sz) {
				move = pop();
				if (move != best && position.isLegal(move)) {
					return move;
				}
			}
			break;

		case AllLegal:
			// All legal
			// Generate and return each legal move.
			while (sz) {
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
MoveList::MoveList(const Position& s, Move bestMove, History* history, int ply, bool qsearch) 
: isQSearch(qsearch), valid(FULL), position(s), history(history), ply(ply), sz(0), best(bestMove), killer1(NULL_MOVE), killer2(NULL_MOVE) {
	if (history) {
		killer1 = history->getKiller(ply).first;
		killer2 = history->getKiller(ply).second;
	}
	if (position.inCheck()) {
		Square k = position.getKingSquare(position.getOurColor());
		Square c = get_lsb(position.getCheckersBB());
		valid = between[k][c] | position.getCheckersBB();
		if (position.inDoubleCheck()) {
			generateMoves<MoveType::All>();
			checkLegal();
			stage = AllLegal;
		}
		else {
			stage = EvadeBestMove;
		}
	}
	else {
		stage = qsearch ? QBestMove : BestMove;
	}
}

/* List of moves and related functions */
MoveList::MoveList(const Position& s) : isQSearch(false), valid(FULL), position(s), history(nullptr), ply(0), sz(0), best(NULL_MOVE) {
	generateMoves<MoveType::All>();
	checkLegal();
	stage = AllLegal;
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