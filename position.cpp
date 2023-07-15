/**
 * Moraband, known in antiquity as Korriban, was an 
 * Outer Rim planet that was home to the ancient Sith 
 **/

#include "position.h"
#include "eval.h"
#include "zobrist.h"
#include <utility>

/* Board position and related functions */
Position::Position(const Position & s)
	: mUs(s.mUs)
	, mThem(s.mThem)
	, mFiftyMoveRule(s.mFiftyMoveRule)
	, mCastleRights(s.mCastleRights)
	, mPhase(s.mPhase)
	, mKey(s.mKey)
	, mPawnKey(s.mPawnKey)
	, mCheckers(s.mCheckers)
	, mEnPassant(s.mEnPassant) 
	, mPreviousMove(s.mPreviousMove)
	, mCheckSquares(s.mCheckSquares)
	, mPinned(s.mPinned)
	, mOccupancy(s.mOccupancy)
	, mPieceIndex(s.mPieceIndex)
	, mBoard(s.mBoard)
	, mPstScore(s.mPstScore)
	, mPieces(s.mPieces)
	, mPieceCount(s.mPieceCount)
	, mPieceList(s.mPieceList)
{}

void Position::operator=(const Position & s) {
	mUs = s.mUs;
	mThem = s.mThem;
	mFiftyMoveRule = s.mFiftyMoveRule;
	mCastleRights = s.mCastleRights;
	mPhase = s.mPhase;
	mKey = s.mKey;
	mPawnKey = s.mPawnKey;
	mCheckers = s.mCheckers;
	mEnPassant = s.mEnPassant;
	mPreviousMove = s.mPreviousMove;
	mCheckSquares = s.mCheckSquares;
	mPinned = s.mPinned;
	mOccupancy = s.mOccupancy;
	mPieceIndex = s.mPieceIndex;
	mBoard = s.mBoard;
	mPieces = s.mPieces;
	mPieceCount = s.mPieceCount;
	mPstScore = s.mPstScore;
	mPieceList = s.mPieceList;
}

Position::Position(const std::string & fen) {
	int i, enpass, position;
	std::string::const_iterator it;
	Square s;
	Color c;
	PieceType p;

	init();

	position = 0;
	for (it = fen.begin(); it < fen.end(); ++it) {
		if (isdigit(*it)) {
			position += *it - '0';
		}
		else if (isalpha(*it)) {
			c = isupper(*it) ? WHITE : BLACK;
			s = last_sq - position;
			char t = tolower(*it);
			p = t == 'p' ? PIECETYPE_PAWN
				: t == 'n' ? PIECETYPE_KNIGHT
					: t == 'b' ? PIECETYPE_BISHOP
						: t == 'r' ? PIECETYPE_ROOK
							: t == 'q' ? PIECETYPE_QUEEN
								: PIECETYPE_KING;
			addPiece(c, p, s);
			mKey ^= Zobrist::key(c, p, s);
			if (p == PIECETYPE_PAWN) {
				mPawnKey ^= Zobrist::key(c, PIECETYPE_PAWN, s);
			}
			position++;
		}
		else if (*it == ' ') {
			++it;
			break;
		}
	}
	if (*it == 'w') {
		mUs = WHITE;
		mThem = BLACK;
	}
	else {
		mUs = BLACK;
		mThem = WHITE;
		mKey ^= Zobrist::key();
	}

	enpass = -1;
	for (++it; it < fen.end(); ++it) {
		if (*it == 'K') {
			mCastleRights += WHITE_KINGSIDE_CASTLE;
		}
		else if (*it == 'Q') {
			mCastleRights += WHITE_QUEENSIDE_CASTLE;
		}
		else if (*it == 'k') {
			mCastleRights += BLACK_KINGSIDE_CASTLE;
		}
		else if (*it == 'q') {
			mCastleRights += BLACK_QUEENSIDE_CASTLE;
		}
		else if (isalpha(*it)) {
			enpass = 'h' - *it;
			++it;
			enpass += 8 * (*it - '1');
		}
	}
	mKey ^= Zobrist::key(mCastleRights);

	if (enpass > -1) {
		mEnPassant = square_bb[enpass];
		mKey ^= Zobrist::key(get_file(mEnPassant));
	}
	
	mPreviousMove = NULL_MOVE;

	// Initialize pins
	setPins(WHITE);
	setPins(BLACK);

	// Initialize checkers
	setCheckers();

	// Initialize game phase
	setGamePhase();
}

void Position::init() {
	mUs = WHITE;
	mThem = BLACK;
	mFiftyMoveRule = 0;
	mCastleRights = 0;
	mKey = 0;
	mPawnKey = 0;
	mCheckers = 0;
	mEnPassant = 0;
	mPreviousMove = NULL_MOVE;
	mCheckSquares.fill({});
	mPinned.fill({});
	mOccupancy.fill({});
	mPieceIndex.fill({});
	mBoard.fill(PIECETYPE_NONE);
	mPieces.fill({});
	mPieceCount.fill({});
	mPstScore.fill({});
	for (auto i = mPieceList.begin(); i != mPieceList.end(); ++i) {
		for (auto j = i->begin(); j != i->end(); ++j) {
			j->fill(no_sq);
		}
	}
}

void Position::setPins(Color c) {
	U64 pinners, ray;
	Square kingSq = getKingSquare(c);
	mPinned[c] = 0;

	pinners = bishopMoves[kingSq] & (getPieceBB<PIECETYPE_BISHOP>(!c) | getPieceBB<PIECETYPE_QUEEN>(!c));

	while (pinners) {
		ray = between_dia[pop_lsb(pinners)][kingSq] & getOccupancyBB();
		if (pop_count(ray) == 1) {
			mPinned[c] |= ray & getOccupancyBB(c);
		}
	}

	pinners = rookMoves[kingSq] & (getPieceBB<PIECETYPE_ROOK>(!c) | getPieceBB<PIECETYPE_QUEEN>(!c));

	while (pinners) {
		ray = between_hor[pop_lsb(pinners)][kingSq] & getOccupancyBB();
		if (pop_count(ray) == 1) {
			mPinned[c] |= ray & getOccupancyBB(c);
		}
	}
}

U64 Position::getDiscoveredChecks(Color c) const {
	U64 pinners, ray, discover = 0;
	Square kingSq = getKingSquare(!c);

	pinners = bishopMoves[kingSq] & (getPieceBB<PIECETYPE_BISHOP>(c) | getPieceBB<PIECETYPE_QUEEN>(c));

	while (pinners) {
		ray = between_dia[pop_lsb(pinners)][kingSq] & getOccupancyBB();
		if (pop_count(ray) == 1) {
			discover |= ray & getOccupancyBB(c);
		}
	}

	pinners = rookMoves[kingSq] & (getPieceBB<PIECETYPE_ROOK>(c) | getPieceBB<PIECETYPE_QUEEN>(c));

	while (pinners) {
		ray = between_hor[pop_lsb(pinners)][kingSq] & getOccupancyBB();
		if (pop_count(ray) == 1) {
			discover |= ray & getOccupancyBB(c);
		}
	}
	
	return discover;
}

bool Position::isLegal(Move move) const {
	Square src = getSrc(move);
	Square dst = getDst(move);

	if (square_bb[src] & mPinned[mUs] && !(coplanar[src][dst] & getPieceBB<PIECETYPE_KING>(mUs))) {
		return false;
	}

	if (onSquare(src) == PIECETYPE_PAWN && square_bb[dst] & mEnPassant) {
		U64 change = mUs == WHITE ? square_bb[src] | square_bb[dst - 8] : square_bb[src] | square_bb[dst + 8];
		if (check(change)) {
			return false;
		}
	}

	if (onSquare(src) == PIECETYPE_KING && !isCastle(move)) {
		U64 change = square_bb[src];
		if (isAttacked(dst, mUs, change)) {
			return false;
		}
	}

	if (dst == getKingSquare(mThem)) {
		return false;
	}

	if ((square_bb[dst] & mOccupancy[mUs]) != 0) {
		return false;
	}

	//assert(dst != getKingSquare(mThem));
	//assert((square_bb[dst] & mOccupancy[mUs]) == 0);
	return true;
}

bool Position::isValid(Move move, U64 validMoves) const {
	assert(getSrc(move) < no_sq);
	assert(getDst(move) < no_sq);

	Square src, dst;
	U64 ray;
	src = getSrc(move);
	dst = getDst(move);

	if (move == NULL_MOVE) {
		return false;
	}
	if (getPiecePromotion(move) && (onSquare(src) != PIECETYPE_PAWN)) {
		return false;
	}
	if (isCastle(move) && onSquare(src) != PIECETYPE_KING) {
		return false;
	}
	if (!(square_bb[src] & getOccupancyBB(mUs)) || (square_bb[dst]  & getOccupancyBB(mUs)) || dst == getKingSquare(mThem)) {
		return false;
	}

	assert(dst != getKingSquare(mThem));
	assert(dst != getKingSquare(mUs));

	switch (onSquare(src)) {
		case PIECETYPE_PAWN:
		{
			if ((square_bb[dst] & RANK_PROMOTION) && !getPiecePromotion(move)) {
				return false;
			}
			if (src < dst == mUs) {
				return false;
			}

			std::pair<Square, Square> advance = std::minmax(src, dst);
			switch (advance.second - advance.first) {
				case 8:
					return square_bb[dst] & getEmptyBB() & validMoves;
				case 16:
					return square_bb[dst] & getEmptyBB() & validMoves && between_hor[src][dst] & getEmptyBB() && square_bb[src] & RANK_PAWN_START;
				case 7:
				case 9:
					return square_bb[dst] & getOccupancyBB(mThem) & validMoves;
				default:
					return false;
			}
		}
		
		case PIECETYPE_KNIGHT:
			return getAttackBB<PIECETYPE_KNIGHT>(src) & square_bb[dst] & validMoves;
		case PIECETYPE_BISHOP:
			return getAttackBB<PIECETYPE_BISHOP>(src) & square_bb[dst] & validMoves;
		case PIECETYPE_ROOK:
			return getAttackBB<PIECETYPE_ROOK>(src) & square_bb[dst] & validMoves;
		case PIECETYPE_QUEEN:
			return getAttackBB<PIECETYPE_QUEEN>(src) & square_bb[dst] & validMoves;

		case PIECETYPE_KING:
		{
			Square k = getKingSquare(mUs);
			if (isCastle(move)) {
				if (src > dst) {
					return (canCastleKingside() && !(between_hor[k][k - 3] & getOccupancyBB()) && !attacked(k - 1) && !attacked(k - 2));
				}
				else {
					return (canCastleQueenside() && !(between_hor[k][k + 4] & getOccupancyBB()) && !attacked(k + 1) && !attacked(k + 2));
				}
			}
			return square_bb[dst];
		}
	}
	
	assert(false);
	return false;
}

bool Position::givesCheck(Move move) const {
	Square src = getSrc(move);
	Square dst = getDst(move);
	PieceType piece = onSquare(src);

	// Direct check
	if (getCheckSquaresBB(piece) & square_bb[dst]) {
		return true;
	}

	// Discovered check
	if ((getDiscoveredChecks(mUs) & square_bb[src]) && !(coplanar[src][dst] & getPieceBB<PIECETYPE_KING>(mThem))) {
		return true;
	}

	if (isEnPassant(move)) {
		U64 change = square_bb[src] | square_bb[dst];
		change |= mUs == WHITE ? square_bb[dst - 8] : square_bb[dst + 8];
		return check(change, mThem);
	}
	else if (isCastle(move)) {
		Square rookSquare = src > dst ? dst + 1 : dst - 1;
		return getAttackBB(PIECETYPE_ROOK, rookSquare, getOccupancyBB() ^ square_bb[src]) & getPieceBB<PIECETYPE_KING>(mThem);
	}
	else if (isPromotion(move)) {
		PieceType promo = getPiecePromotion(move);
		return getAttackBB(promo, dst, getOccupancyBB() ^ square_bb[src]) &	getPieceBB<PIECETYPE_KING>(mThem);
	}
	
	return false;
}

int Position::see(Move move) const {
	Prop prop;
	Color color;
	Square src, dst, mayAttack;
	PieceType target;
	U64 attackers, from, occupancy, xRay, potential;
	int gain[32] = { 0 };
	int d = 0;

	color = mUs;
	src = getSrc(move);
	dst = getDst(move);
	from = square_bb[src];
	// Check if the move is en passant
	if (onSquare(src) == PIECETYPE_PAWN && square_bb[dst] & mEnPassant) {
		gain[d] = PieceValue[PIECETYPE_PAWN];
	}
	else if (getPiecePromotion(move) == PIECETYPE_QUEEN) {
		gain[d] = QUEEN_WEIGHT_MG - PAWN_WEIGHT_MG;
	}
	else {
		gain[d] = PieceValue[onSquare(dst)];
	}
	
	occupancy = getOccupancyBB();
	attackers = allAttackers(dst);
	// Get X ray attacks and add in pawns from attackers
	xRay = getXRayAttacks(dst);
	
	while (from) {
		// Update the target piece and the square it came from
		src = get_lsb(from);
		target = onSquare(src);

		// Update the depth and color
		d++;

		// Storing the potential gain, if defended
		gain[d] = PieceValue[target] - gain[d - 1];

		// Prune if the gain cannot be improved
		if (std::max(-gain[d - 1], gain[d]) < 0) {
			break;
		}

		// Remove the from bit to simulate making the move
		attackers ^= from;
		occupancy ^= from;

		if (target != PIECETYPE_KNIGHT) {
			xRay &= occupancy;
			potential = coplanar[src][dst] & xRay;
			while (potential) {
				mayAttack = pop_lsb(potential);
				if (!(between[mayAttack][dst] & occupancy)) {
					attackers |= square_bb[mayAttack];
					break;
				}
			}
		}

		// Get the next piece for the opposing player
		color = !color;
		if (!(attackers & getOccupancyBB(color))) {
			break;
		}

		if (attackers & getPieceBB<PIECETYPE_PAWN>(color)) {
			from = get_lsb_bb(attackers & getPieceBB<PIECETYPE_PAWN>(color));
		}
		else if (attackers & getPieceBB<PIECETYPE_KNIGHT>(color)) {
			from = get_lsb_bb(attackers & getPieceBB<PIECETYPE_KNIGHT>(color));
		}
		else if (attackers & getPieceBB<PIECETYPE_BISHOP>(color)) {
			from = get_lsb_bb(attackers & getPieceBB<PIECETYPE_BISHOP>(color));
		}
		else if (attackers & getPieceBB<PIECETYPE_ROOK>(color)) {
			from = get_lsb_bb(attackers & getPieceBB<PIECETYPE_ROOK>(color));
		}
		else if (attackers & getPieceBB<PIECETYPE_QUEEN>(color)) {
			from = get_lsb_bb(attackers & getPieceBB<PIECETYPE_QUEEN>(color));
		}
		else {
			from = get_lsb_bb(attackers & getPieceBB<PIECETYPE_KING>(color));
			if (attackers & getOccupancyBB(!color)) {
				break;
			}
		}
	}

	// Negamax the gain array to determine the final SEE value.
	while (--d > 0) {
		gain[d - 1] = -std::max(-gain[d - 1], gain[d]);
	}

	return gain[0];
}

void Position::makeMove(Move move) {
	assert(move != NULL_MOVE);
	assert(getSrc(move) < no_sq);
	assert(getDst(move) < no_sq);
	Square src, dst;
	PieceType moved, captured;
	bool epFlag = false;
	bool gamePhase = false;

	src = getSrc(move);
	dst = getDst(move);
	moved = onSquare(src);
	assert(moved != PIECETYPE_NONE);
	captured = onSquare(dst);
	assert(captured != PIECETYPE_KING);

	// Update the Fifty Move Rule
	mFiftyMoveRule++;

	// Remove the ep file and castle rights from the zobrist key.
	if (mEnPassant) {
		mKey ^= Zobrist::key(get_file(mEnPassant));
	}
	mKey ^= Zobrist::key(mCastleRights);

	if (captured != PIECETYPE_NONE && !isCastle(move)) {
		mFiftyMoveRule = 0;
		removePiece(mThem, captured, dst);
		if (captured == PIECETYPE_PAWN) {
			mPawnKey ^= Zobrist::key(mThem, PIECETYPE_PAWN, dst);
		}
		gamePhase = true;
	}

	if (isCastle(move)) {
		// Short castle
		if (dst < src) {
			movePiece(mUs, PIECETYPE_ROOK, src-3, dst+1);
			movePiece(mUs, PIECETYPE_KING, src, dst);
		}
		// Long castle
		else {
			movePiece(mUs, PIECETYPE_ROOK, src+4, dst-1);
			movePiece(mUs, PIECETYPE_KING, src, dst);
		}
	}
	else {
		movePiece(mUs, moved, src, dst);
	}
	
	if (moved == PIECETYPE_PAWN) {
		mFiftyMoveRule = 0;
		mPawnKey ^= Zobrist::key(mUs, PIECETYPE_PAWN, src, dst);

		// Check for double PIECETYPE_PAWN push
		if (int(std::max(src, dst)) - int(std::min(src, dst)) == 16) {
			mEnPassant = mUs == WHITE ? square_bb[dst - 8] : square_bb[dst + 8];

			mKey ^= Zobrist::key(get_file(mEnPassant));
			epFlag = true;
		}
		else if (getPiecePromotion(move)) {
			mPawnKey ^= Zobrist::key(mUs, PIECETYPE_PAWN, dst);
			removePiece(mUs, PIECETYPE_PAWN, dst);
			addPiece(mUs, getPiecePromotion(move), dst);
			gamePhase = true;
		}
		else if (mEnPassant & square_bb[dst]) {
			Square epCapture = (mUs == WHITE) ? dst - 8 : dst + 8;
			mPawnKey ^= Zobrist::key(mThem, PIECETYPE_PAWN, epCapture);
			removePiece(mThem, PIECETYPE_PAWN, epCapture);
			gamePhase = true;
		}
	}

	if (!epFlag) {
		mEnPassant = 0;
	}
	if (gamePhase) {
		setGamePhase();
	}
	// Update castle rights
	mCastleRights &= CASTLE_RIGHTS[src];
	mCastleRights &= CASTLE_RIGHTS[dst];

	mKey ^= Zobrist::key(mCastleRights);

	assert(!check());
	assert((mEnPassant & getOccupancyBB()) == 0);
	mPreviousMove = move;
	swapTurn();

	setPins(WHITE);
	setPins(BLACK);
	setCheckers();
}

void Position::makeNull() {
	assert(mCheckers == 0);
	// Remove the ep file and castle rights from the zobrist key
	if (mEnPassant) {
		mKey ^= Zobrist::key(get_file(mEnPassant));
	}
	// Reset the en-passant square
	mEnPassant = 0;

	// Swap the turn
	swapTurn();

	// Set check squares
	mCheckSquares[PIECETYPE_PAWN] = getAttackBB<PIECETYPE_PAWN>(getKingSquare(mThem), mThem);
	mCheckSquares[PIECETYPE_KNIGHT] = getAttackBB<PIECETYPE_KNIGHT>(getKingSquare(mThem));
	mCheckSquares[PIECETYPE_BISHOP] = getAttackBB<PIECETYPE_BISHOP>(getKingSquare(mThem));
	mCheckSquares[PIECETYPE_ROOK] = getAttackBB<PIECETYPE_ROOK>(getKingSquare(mThem));
	mCheckSquares[PIECETYPE_QUEEN] = mCheckSquares[PIECETYPE_BISHOP] | mCheckSquares[PIECETYPE_ROOK];
}

bool Position::insufficientMaterial() const {
	bool res = false;
	
	if (getPieceCount<PIECETYPE_PAWN>() + getPieceCount<PIECETYPE_ROOK>() + getPieceCount<PIECETYPE_QUEEN>() == 0) {
		switch (getPieceCount<PIECETYPE_KNIGHT>()) {
			case 0:
				if ((getPieceBB<PIECETYPE_BISHOP>() & DARK_SQUARES) == getPieceBB<PIECETYPE_BISHOP>() || (getPieceBB<PIECETYPE_BISHOP>() & LIGHT_SQUARES) == getPieceBB<PIECETYPE_BISHOP>()) {
					res = true;
				}
				break;
				               
			case 1:
				if (!getPieceCount<PIECETYPE_BISHOP>()) {
					res = true;
				}
				break;
				
			case 2:
				if (getPieceCount<PIECETYPE_KNIGHT>(WHITE) && getPieceCount<PIECETYPE_KNIGHT>(BLACK) && !getPieceCount<PIECETYPE_BISHOP>())
					res = true;
				break;
				
			default:
				break;
		}
	}
	return res;
}

std::string Position::getFen() { // Current FEN
	Square src = getSrc(getPreviousMove());
	Square dst = getDst(getPreviousMove());
	PieceType moved = onSquare(dst);
	
	std::string fen = "";
	int e = 0;
	for (int i = 63; i >= 0; --i) {
		U64 bit = square_bb[i];
		if (i % 8 == 7 && i != 63) {
			if (e != 0) {
				fen += e + '0';
				e = 0;
			}
			fen += '/';
		}
		
		char p = bit & getPieceBB<PIECETYPE_PAWN>(WHITE) ? 'P'
			: bit & getPieceBB<PIECETYPE_KNIGHT>(WHITE) ? 'N'
				: bit & getPieceBB<PIECETYPE_BISHOP>(WHITE) ? 'B'
					: bit & getPieceBB<PIECETYPE_ROOK>(WHITE) ? 'R'
						: bit & getPieceBB<PIECETYPE_QUEEN>(WHITE) ? 'Q'
							: bit & getPieceBB<PIECETYPE_KING>(WHITE) ? 'K'
								: bit & getPieceBB<PIECETYPE_PAWN>(BLACK) ? 'p'
									: bit & getPieceBB<PIECETYPE_KNIGHT>(BLACK) ? 'n'
										: bit & getPieceBB<PIECETYPE_BISHOP>(BLACK) ? 'b'
											: bit & getPieceBB<PIECETYPE_ROOK>(BLACK) ? 'r'
												: bit & getPieceBB<PIECETYPE_QUEEN>(BLACK) ? 'q'
													: bit & getPieceBB<PIECETYPE_KING>(BLACK) ? 'k' 
														: 'e';
		if (p == 'e') {
			++e;
		}
		else {
			if (e != 0) {
				fen += e + '0';
				e = 0;
			}
			fen += p;
		}
	}
	if (e != 0) {
		fen += e + '0';
	}
	fen += ' ';
	if (mUs == WHITE) {
		fen += 'w';
	}
	else {
		fen += 'b';
	}
	fen += ' ';
	bool castle = false;
	if (canCastleKingside(WHITE)) {
		fen += 'K';
		castle = true;
	}
	if (canCastleQueenside(WHITE)) {
		fen += 'Q';
		castle = true;
	}
	if (canCastleKingside(BLACK)) {
		fen += 'k';
		castle = true;
	} 
	if (canCastleQueenside(BLACK)) {
		fen += 'q';
		castle = true;
	}
	
	if (!castle) {
		fen += '-';
	}
	fen += ' ';
	
	// Check move could result in enpassant
	bool enpass = false;
	if (moved == PIECETYPE_PAWN && int(std::max(src, dst)) - int(std::min(src, dst)) == 16) {
		for (Square p : getPieceList<PIECETYPE_PAWN>(mUs)) {
			if (rank(p) == rank(dst) && std::max(file(p), file(dst)) - std::min(file(p), file(dst)) == 1) {
				fen += SQUARE_TO_STRING[dst + (mUs == WHITE ? 8 : -8)];
				enpass = true;
				break;
			}
		}
	}
	if (!enpass) {
		fen += '-';
	}
	
	return fen;
}

// Print board
std::ostream & operator << (std::ostream & os, const Position & s) {
	static const char * W_pawn   = "\u2659";
	static const char * W_knight = "\u2658";
	static const char * W_bishop = "\u2657";
	static const char * W_rook   = "\u2656";
	static const char * W_queen  = "\u2655";
	static const char * W_king   = "\u2654";
	static const char * B_pawn   = "\u265F";
	static const char * B_knight = "\u265E";
	static const char * B_bishop = "\u265D";
	static const char * B_rook   = "\u265C";
	static const char * B_queen  = "\u265B";
	static const char * B_king   = "\u265A";
	static const char * Empty    = " ";
	
	std::string nums[8] = {"1", "2", "3", "4", "5", "6", "7", "8"};
	const std::string bar = "  + - + - + - + - + - + - + - + - + ";

	os << bar << std::endl;
	for (int i = 63; i >= 0; --i) {
		U64 bit = square_bb[i];
		if (i % 8 == 7) {
			os << nums[i / 8] << " | ";
		}
		
		os << (bit & s.getPieceBB<PIECETYPE_PAWN>(WHITE) ? W_pawn
			: bit & s.getPieceBB<PIECETYPE_KNIGHT>(WHITE) ? W_knight
				: bit & s.getPieceBB<PIECETYPE_BISHOP>(WHITE) ? W_bishop
					: bit & s.getPieceBB<PIECETYPE_ROOK>(WHITE) ? W_rook
						: bit & s.getPieceBB<PIECETYPE_QUEEN>(WHITE) ? W_queen
							: bit & s.getPieceBB<PIECETYPE_KING>(WHITE) ? W_king
								: bit & s.getPieceBB<PIECETYPE_PAWN>(BLACK) ? B_pawn
									: bit & s.getPieceBB<PIECETYPE_KNIGHT>(BLACK) ? B_knight
										: bit & s.getPieceBB<PIECETYPE_BISHOP>(BLACK) ? B_bishop
											: bit & s.getPieceBB<PIECETYPE_ROOK>(BLACK) ? B_rook
												: bit & s.getPieceBB<PIECETYPE_QUEEN>(BLACK) ? B_queen
													: bit & s.getPieceBB<PIECETYPE_KING>(BLACK) ? B_king
														: Empty) << " | ";
		
		if (i % 8 == 0) {
			os << '\n' << bar << '\n';
		}
	}

	os << "    a   b   c   d   e   f   g   h\n";
	
	os << "Key:     " << s.mKey << '\n';
	os << "PawnKey: " << s.mPawnKey << '\n';
	os << "Phase:   " << s.getGamePhase() << '\n';
	os << "previous move: " << to_string(s.mPreviousMove) << '\n';
	if (s.mUs == WHITE) {
		os << "White to move\n";
	}
	else {
		os << "Black to move\n";
	}
	
	return os;
}