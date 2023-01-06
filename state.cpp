#include "state.h"
#include "zobrist.h"
#include <utility>

State::State(const State & s)
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

void State::operator=(const State & s) {
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

State::State(const std::string & fen) {
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
			p = t == 'p' ? pawn
				: t == 'n' ? knight
					: t == 'b' ? bishop
						: t == 'r' ? rook
							: t == 'q' ? queen
								: king;
			addPiece(c, p, s);
			mKey ^= Zobrist::key(c, p, s);
			if (p == pawn) {
				mPawnKey ^= Zobrist::key(c, pawn, s);
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

void State::init() {
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
	mBoard.fill(none);
	mPieces.fill({});
	mPieceCount.fill({});
	mPstScore.fill({});
	for (auto i = mPieceList.begin(); i != mPieceList.end(); ++i) {
		for (auto j = i->begin(); j != i->end(); ++j) {
			j->fill(no_sq);
		}
	}
}

void State::setPins(Color c) {
	U64 pinners, ray;
	Square kingSq = getKingSquare(c);
	mPinned[c] = 0;

	pinners = bishopMoves[kingSq] & (getPieceBB<bishop>(!c) | getPieceBB<queen>(!c));

	while (pinners) {
		ray = between_dia[pop_lsb(pinners)][kingSq] & getOccupancyBB();
		if (pop_count(ray) == 1) {
			mPinned[c] |= ray & getOccupancyBB(c);
		}
	}

	pinners = rookMoves[kingSq] & (getPieceBB<rook>(!c) | getPieceBB<queen>(!c));

	while (pinners) {
		ray = between_hor[pop_lsb(pinners)][kingSq] & getOccupancyBB();
		if (pop_count(ray) == 1) {
			mPinned[c] |= ray & getOccupancyBB(c);
		}
	}
}

U64 State::getDiscoveredChecks(Color c) const {
	U64 pinners, ray, discover = 0;
	Square kingSq = getKingSquare(!c);

	pinners = bishopMoves[kingSq] & (getPieceBB<bishop>(c) | getPieceBB<queen>(c));

	while (pinners) {
		ray = between_dia[pop_lsb(pinners)][kingSq] & getOccupancyBB();
		if (pop_count(ray) == 1) {
			discover |= ray & getOccupancyBB(c);
		}
	}

	pinners = rookMoves[kingSq] & (getPieceBB<rook>(c) | getPieceBB<queen>(c));

	while (pinners) {
		ray = between_hor[pop_lsb(pinners)][kingSq] & getOccupancyBB();
		if (pop_count(ray) == 1) {
			discover |= ray & getOccupancyBB(c);
		}
	}
	
	return discover;
}

bool State::isLegal(Move pMove) const {
	Square src = getSrc(pMove);
	Square dst = getDst(pMove);

	if (square_bb[src] & mPinned[mUs] && !(coplanar[src][dst] & getPieceBB<king>(mUs))) {
		return false;
	}

	if (onSquare(src) == pawn && square_bb[dst] & mEnPassant) {
		U64 change = mUs == WHITE ? square_bb[src] | square_bb[dst - 8] : square_bb[src] | square_bb[dst + 8];
		if (check(change)) {
			return false;
		}
	}

	if (onSquare(src) == king && !isCastle(pMove)) {
		U64 change = square_bb[src];
		if (isAttacked(dst, mUs, change)) {
			return false;
		}
	}

	assert(dst != getKingSquare(mThem));
	assert((square_bb[dst] & mOccupancy[mUs]) == 0);
	return true;
}

bool State::isValid(Move pMove, U64 pValidMoves) const {
	assert(getSrc(pMove) < no_sq);
	assert(getDst(pMove) < no_sq);

	Square src, dst;
	U64 ray;
	src = getSrc(pMove);
	dst = getDst(pMove);

	if (pMove == NULL_MOVE) {
		return false;
	}
	if (getPiecePromotion(pMove) && (onSquare(src) != pawn)) {
		return false;
	}
	if (isCastle(pMove) && onSquare(src) != king) {
		return false;
	}
	if (!(square_bb[src] & getOccupancyBB(mUs)) || (square_bb[dst]  & getOccupancyBB(mUs)) || dst == getKingSquare(mThem)) {
		return false;
	}

	assert(dst != getKingSquare(mThem));
	assert(dst != getKingSquare(mUs));

	switch (onSquare(src)) {
		case pawn:
		{
			if ((square_bb[dst] & RANK_PROMOTION) && !getPiecePromotion(pMove)) {
				return false;
			}
			if (src < dst == mUs) {
				return false;
			}

			std::pair<Square, Square> advance = std::minmax(src, dst);
			switch (advance.second - advance.first) {
				case 8:
					return square_bb[dst] & getEmptyBB() & pValidMoves;
				case 16:
					return square_bb[dst] & getEmptyBB() & pValidMoves && between_hor[src][dst] & getEmptyBB() && square_bb[src] & RANK_PAWN_START;
				case 7:
				case 9:
					return square_bb[dst] & getOccupancyBB(mThem) & pValidMoves;
				default:
					return false;
			}
		}
		
		case knight:
			return getAttackBB<knight>(src) & square_bb[dst] & pValidMoves;
		case bishop:
			return getAttackBB<bishop>(src) & square_bb[dst] & pValidMoves;
		case rook:
			return getAttackBB<rook>(src) & square_bb[dst] & pValidMoves;
		case queen:
			return getAttackBB<queen>(src) & square_bb[dst] & pValidMoves;

		case king:
		{
			Square k = getKingSquare(mUs);
			if (isCastle(pMove)) {
				if (src > dst) {
					return (canCastleKingside() && !(between_hor[k][k-3] & getOccupancyBB()) && !attacked(k-1) && !attacked(k-2));
				}
				else {
					return (canCastleQueenside() && !(between_hor[k][k+4] & getOccupancyBB()) && !attacked(k+1) && !attacked(k+2));
				}
			}
			return square_bb[dst];
		}
	}
	
	assert(false);
	return false;
}

bool State::givesCheck(Move pMove) const {
	Square src = getSrc(pMove);
	Square dst = getDst(pMove);
	PieceType piece = onSquare(src);

	// Direct check
	if (getCheckSquaresBB(piece) & square_bb[dst]) {
		return true;
	}

	// Discovered check
	if ((getDiscoveredChecks(mUs) & square_bb[src]) && !(coplanar[src][dst] & getPieceBB<king>(mThem))) {
		return true;
	}

	if (isEnPassant(pMove)) {
		U64 change = square_bb[src] | square_bb[dst];
		change |= mUs == WHITE ? square_bb[dst - 8] : square_bb[dst + 8];
		return check(change, mThem);
	}
	else if (isCastle(pMove)) {
		Square rookSquare = src > dst ? dst + 1 : dst - 1;
		return getAttackBB(rook, rookSquare, getOccupancyBB() ^ square_bb[src]) & getPieceBB<king>(mThem);
	}
	else if (isPromotion(pMove)) {
		PieceType promo = getPiecePromotion(pMove);
		return getAttackBB(promo, dst, getOccupancyBB() ^ square_bb[src]) &	getPieceBB<king>(mThem);
	}
	
	return false;
}

int State::see(Move m) const {
	Prop prop;
	Color color;
	Square src, dst, mayAttack;
	PieceType target;
	U64 attackers, from, occupancy, xRay, potential;
	int gain[32] = { 0 };
	int d = 0;

	color = mUs;
	src = getSrc(m);
	dst = getDst(m);
	from = square_bb[src];
	// Check if the move is en passant
	if (onSquare(src) == pawn && square_bb[dst] & mEnPassant) {
		gain[d] = PieceValue[pawn];
	}
	else if (getPiecePromotion(m) == queen) {
		gain[d] = QUEEN_WEIGHT - PAWN_WEIGHT;
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

		if (target != knight) {
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

		if (attackers & getPieceBB<pawn>(color)) {
			from = get_lsb_bb(attackers & getPieceBB<pawn>(color));
		}
		else if (attackers & getPieceBB<knight>(color)) {
			from = get_lsb_bb(attackers & getPieceBB<knight>(color));
		}
		else if (attackers & getPieceBB<bishop>(color)) {
			from = get_lsb_bb(attackers & getPieceBB<bishop>(color));
		}
		else if (attackers & getPieceBB<rook>(color)) {
			from = get_lsb_bb(attackers & getPieceBB<rook>(color));
		}
		else if (attackers & getPieceBB<queen>(color)) {
			from = get_lsb_bb(attackers & getPieceBB<queen>(color));
		}
		else {
			from = get_lsb_bb(attackers & getPieceBB<king>(color));
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

void State::makeMove(Move pMove) {
	assert(pMove != NULL_MOVE);
	assert(getSrc(pMove) < no_sq);
	assert(getDst(pMove) < no_sq);
	Square src, dst;
	PieceType moved, captured;
	bool epFlag = false;
	bool gamePhase = false;

	src = getSrc(pMove);
	dst = getDst(pMove);
	moved = onSquare(src);
	assert(moved != none);
	captured = onSquare(dst);
	assert(captured != king);

	// Update the Fifty Move Rule
	mFiftyMoveRule++;

	// Remove the ep file and castle rights from the zobrist key.
	if (mEnPassant) {
		mKey ^= Zobrist::key(get_file(mEnPassant));
	}
	mKey ^= Zobrist::key(mCastleRights);

	if (captured != none && !isCastle(pMove)) {
		mFiftyMoveRule = 0;
		removePiece(mThem, captured, dst);
		if (captured == pawn) {
			mPawnKey ^= Zobrist::key(mThem, pawn, dst);
		}
		gamePhase = true;
	}

	if (isCastle(pMove)) {
		// Short castle
		if (dst < src) {
			movePiece(mUs, rook, src-3, dst+1);
			movePiece(mUs, king, src, dst);
		}
		// Long castle
		else {
			movePiece(mUs, rook, src+4, dst-1);
			movePiece(mUs, king, src, dst);
		}
	}
	else {
		movePiece(mUs, moved, src, dst);
	}
	
	if (moved == pawn) {
		mFiftyMoveRule = 0;
		mPawnKey ^= Zobrist::key(mUs, pawn, src, dst);

		// Check for double pawn push
		if (int(std::max(src, dst)) - int(std::min(src, dst)) == 16) {
			mEnPassant = mUs == WHITE ? square_bb[dst - 8] : square_bb[dst + 8];

			mKey ^= Zobrist::key(get_file(mEnPassant));
			epFlag = true;
		}
		else if (getPiecePromotion(pMove)) {
			mPawnKey ^= Zobrist::key(mUs, pawn, dst);
			removePiece(mUs, pawn, dst);
			addPiece(mUs, getPiecePromotion(pMove), dst);
			gamePhase = true;
		}
		else if (mEnPassant & square_bb[dst]) {
			Square epCapture = (mUs == WHITE) ? dst - 8 : dst + 8;
			mPawnKey ^= Zobrist::key(mThem, pawn, epCapture);
			removePiece(mThem, pawn, epCapture);
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
	mPreviousMove = pMove;
	swapTurn();

	setPins(WHITE);
	setPins(BLACK);
	setCheckers();
}

void State::makeNull() {
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
	mCheckSquares[pawn] = getAttackBB<pawn>(getKingSquare(mThem), mThem);
	mCheckSquares[knight] = getAttackBB<knight>(getKingSquare(mThem));
	mCheckSquares[bishop] = getAttackBB<bishop>(getKingSquare(mThem));
	mCheckSquares[rook] = getAttackBB<rook>(getKingSquare(mThem));
	mCheckSquares[queen] = mCheckSquares[bishop] | mCheckSquares[rook];
}

bool State::insufficientMaterial() const {
	bool res = false;
	
	if (getPieceCount<pawn>() + getPieceCount<rook>() + getPieceCount<queen>() == 0) {
		switch (getPieceCount<knight>()) {
			case 0:
				if ((getPieceBB<bishop>() & DARK_SQUARES) == getPieceBB<bishop>() || (getPieceBB<bishop>() & LIGHT_SQUARES) == getPieceBB<bishop>()) {
					res = true;
				}
				break;
				               
			case 1:
				if (!getPieceCount<bishop>()) {
					res = true;
				}
				break;
				
			case 2:
				if (getPieceCount<knight>(WHITE) && getPieceCount<knight>(BLACK) && !getPieceCount<bishop>())
					res = true;
				break;
				
			default:
				break;
		}
	}
	return res;
}

std::string State::getFen() { // Current FEN
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
		
		char p = bit & getPieceBB<pawn>(WHITE) ? 'P'
			: bit & getPieceBB<knight>(WHITE) ? 'N'
				: bit & getPieceBB<bishop>(WHITE) ? 'B'
					: bit & getPieceBB<rook>(WHITE) ? 'R'
						: bit & getPieceBB<queen>(WHITE) ? 'Q'
							: bit & getPieceBB<king>(WHITE) ? 'K'
								: bit & getPieceBB<pawn>(BLACK) ? 'p'
									: bit & getPieceBB<knight>(BLACK) ? 'n'
										: bit & getPieceBB<bishop>(BLACK) ? 'b'
											: bit & getPieceBB<rook>(BLACK) ? 'r'
												: bit & getPieceBB<queen>(BLACK) ? 'q'
													: bit & getPieceBB<king>(BLACK) ? 'k' 
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
	
	// Check pMove could result in enpassant
	bool enpass = false;
	if (moved == pawn && int(std::max(src, dst)) - int(std::min(src, dst)) == 16) {
		// Double pawn push
		for (Square p : getPieceList<pawn>(mUs)) {
			if (rank(p) == rank(dst) && std::max(file(p), file(dst)) - std::min(file(p), file(dst)) == 1) {
				D(std::cout << SQUARE_TO_STRING[p] << ' ' << SQUARE_TO_STRING[dst] << std::endl;);
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
std::ostream & operator << (std::ostream & os, const State & s) {
	const char * W_pawn   = "\u2659";
	const char * W_knight = "\u2658";
	const char * W_bishop = "\u2657";
	const char * W_rook   = "\u2656";
	const char * W_queen  = "\u2655";
	const char * W_king   = "\u2654";
	const char * B_pawn   = "\u265F";
	const char * B_knight = "\u265E";
	const char * B_bishop = "\u265D";
	const char * B_rook   = "\u265C";
	const char * B_queen  = "\u265B";
	const char * B_king   = "\u265A";
	const char * Empty    = " ";
	
	std::string nums[8] = {"1", "2", "3", "4", "5", "6", "7", "8"};
	const std::string bar = "  + - + - + - + - + - + - + - + - + ";

	os << bar << std::endl;
	for (int i = 63; i >= 0; --i) {
		U64 bit = square_bb[i];
		if (i % 8 == 7) {
			os << nums[i / 8] << " | ";
		}
		
		os << (bit & s.getPieceBB<pawn>(WHITE) ? W_pawn
			: bit & s.getPieceBB<knight>(WHITE) ? W_knight
				: bit & s.getPieceBB<bishop>(WHITE) ? W_bishop
					: bit & s.getPieceBB<rook>(WHITE) ? W_rook
						: bit & s.getPieceBB<queen>(WHITE) ? W_queen
							: bit & s.getPieceBB<king>(WHITE) ? W_king
								: bit & s.getPieceBB<pawn>(BLACK) ? B_pawn
									: bit & s.getPieceBB<knight>(BLACK) ? B_knight
										: bit & s.getPieceBB<bishop>(BLACK) ? B_bishop
											: bit & s.getPieceBB<rook>(BLACK) ? B_rook
												: bit & s.getPieceBB<queen>(BLACK) ? B_queen
													: bit & s.getPieceBB<king>(BLACK) ? B_king
														: Empty) << " | ";
		
		if (i % 8 == 0) {
			os << '\n' << bar << '\n';
		}
	}

	os << "    a   b   c   d   e   f   g   h\n";
	
	// os << "mKey:     " << s.mKey << '\n';
	// os << "mPawnKey: " << s.mPawnKey << '\n';
	os << "previous move: " << to_string(s.mPreviousMove) << '\n';
	if (s.mUs == WHITE) {
		os << "White to move\n";
	}
	else {
		os << "Black to move\n";
	}
	
	return os;
}
