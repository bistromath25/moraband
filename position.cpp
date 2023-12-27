/**
 * Moraband, known in antiquity as Korriban, was an 
 * Outer Rim planet that was home to the ancient Sith 
 **/

#include "position.h"
#include "eval.h"
#include "zobrist.h"
#include <utility>

/* Board position and related functions */
Position::Position(const Position &s)
	: us(s.us)
	, them(s.them)
	, fiftyMoveRule(s.fiftyMoveRule)
	, castleRights(s.castleRights)
	, phase(s.phase)
	, key(s.key)
	, pawnKey(s.pawnKey)
	, checkers(s.checkers)
	, enPassant(s.enPassant) 
	, previousMove(s.previousMove)
	, checkSquares(s.checkSquares)
	, pinned(s.pinned)
	, occupancy(s.occupancy)
	, pieceIndex(s.pieceIndex)
	, board(s.board)
	, pieces(s.pieces)
	, pieceCounts(s.pieceCounts)
	, pstScore(s.pstScore)
	, pieceList(s.pieceList)
{}

void Position::operator=(const Position &s) {
	us = s.us;
	them = s.them;
	fiftyMoveRule = s.fiftyMoveRule;
	castleRights = s.castleRights;
	phase = s.phase;
	key = s.key;
	pawnKey = s.pawnKey;
	checkers = s.checkers;
	enPassant = s.enPassant;
	previousMove = s.previousMove;
	checkSquares = s.checkSquares;
	pinned = s.pinned;
	occupancy = s.occupancy;
	pieceIndex = s.pieceIndex;
	board = s.board;
	pieces = s.pieces;
	pieceCounts = s.pieceCounts;
	pstScore = s.pstScore;
	pieceList = s.pieceList;
}

Position::Position(const std::string &fen) {
	int enpass, position;
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
			key ^= Zobrist::key(c, p, s);
			if (p == PIECETYPE_PAWN) {
				pawnKey ^= Zobrist::key(c, PIECETYPE_PAWN, s);
			}
			position++;
		}
		else if (*it == ' ') {
			++it;
			break;
		}
	}
	if (*it == 'w') {
		us = WHITE;
		them = BLACK;
	}
	else {
		us = BLACK;
		them = WHITE;
		key ^= Zobrist::key();
	}

	enpass = -1;
	for (++it; it < fen.end(); ++it) {
		if (*it == 'K') {
			castleRights += WHITE_KINGSIDE_CASTLE;
		}
		else if (*it == 'Q') {
			castleRights += WHITE_QUEENSIDE_CASTLE;
		}
		else if (*it == 'k') {
			castleRights += BLACK_KINGSIDE_CASTLE;
		}
		else if (*it == 'q') {
			castleRights += BLACK_QUEENSIDE_CASTLE;
		}
		else if (isalpha(*it)) {
			enpass = 'h' - *it;
			++it;
			enpass += 8 * (*it - '1');
		}
	}
	key ^= Zobrist::key(castleRights);

	if (enpass > -1) {
		enPassant = square_bb[enpass];
		key ^= Zobrist::key(get_file(enPassant));
	}
	
	previousMove = NULL_MOVE;

	// Initialize pins
	setPins(WHITE);
	setPins(BLACK);

	// Initialize checkers
	setCheckers();

	// Initialize game phase
	setGamePhase();
}

void Position::init() {
	us = WHITE;
	them = BLACK;
	fiftyMoveRule = 0;
	castleRights = 0;
	key = 0;
	pawnKey = 0;
	checkers = 0;
	enPassant = 0;
	previousMove = NULL_MOVE;
	checkSquares.fill({});
	pinned.fill({});
	occupancy.fill({});
	pieceIndex.fill({});
	board.fill(PIECETYPE_NONE);
	pieces.fill({});
	pieceCounts.fill({});
	pstScore.fill({});
	for (auto i = pieceList.begin(); i != pieceList.end(); ++i) {
		for (auto j = i->begin(); j != i->end(); ++j) {
			j->fill(no_sq);
		}
	}
}

void Position::setPins(Color c) {
	U64 pinners, ray;
	Square kingSq = getKingSquare(c);
	pinned[c] = 0;

	pinners = bishopMoves[kingSq] & (getPieceBB<PIECETYPE_BISHOP>(!c) | getPieceBB<PIECETYPE_QUEEN>(!c));

	while (pinners) {
		ray = between_dia[pop_lsb(pinners)][kingSq] & getOccupancyBB();
		if (pop_count(ray) == 1) {
			pinned[c] |= ray & getOccupancyBB(c);
		}
	}

	pinners = rookMoves[kingSq] & (getPieceBB<PIECETYPE_ROOK>(!c) | getPieceBB<PIECETYPE_QUEEN>(!c));

	while (pinners) {
		ray = between_hor[pop_lsb(pinners)][kingSq] & getOccupancyBB();
		if (pop_count(ray) == 1) {
			pinned[c] |= ray & getOccupancyBB(c);
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

	if (square_bb[src] & pinned[us] && !(coplanar[src][dst] & getPieceBB<PIECETYPE_KING>(us))) {
		return false;
	}

	if (onSquare(src) == PIECETYPE_PAWN && square_bb[dst] & enPassant) {
		U64 change = us == WHITE ? square_bb[src] | square_bb[dst - 8] : square_bb[src] | square_bb[dst + 8];
		if (check(change)) {
			return false;
		}
	}

	if (onSquare(src) == PIECETYPE_KING && !isCastle(move)) {
		U64 change = square_bb[src];
		if (isAttacked(dst, us, change)) {
			return false;
		}
		return chebyshevDistance(src, dst) == 1;
	}

	if (dst == getKingSquare(them)) {
		return false;
	}

	if ((square_bb[dst] & occupancy[us]) != 0) {
		return false;
	}

	//assert(dst != getKingSquare(them));
	//assert((square_bb[dst] & occupancy[us]) == 0);
	return true;
}

bool Position::isValid(Move move, U64 validMoves) const {
	assert(getSrc(move) < no_sq);
	assert(getDst(move) < no_sq);

	Square src, dst;
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
	if (!(square_bb[src] & getOccupancyBB(us)) || (square_bb[dst]  & getOccupancyBB(us)) || dst == getKingSquare(them)) {
		return false;
	}

	assert(dst != getKingSquare(them));
	assert(dst != getKingSquare(us));

	switch (onSquare(src)) {
		case PIECETYPE_PAWN:
		{
			if ((square_bb[dst] & RANK_PROMOTION) && !getPiecePromotion(move)) {
				return false;
			}
			if ((src < dst) == us) {
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
					return square_bb[dst] & getOccupancyBB(them) & validMoves;
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
			Square k = getKingSquare(us);
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
	if ((getDiscoveredChecks(us) & square_bb[src]) && !(coplanar[src][dst] & getPieceBB<PIECETYPE_KING>(them))) {
		return true;
	}

	if (isEnPassant(move)) {
		U64 change = square_bb[src] | square_bb[dst];
		change |= us == WHITE ? square_bb[dst - 8] : square_bb[dst + 8];
		return check(change, them);
	}
	else if (isCastle(move)) {
		Square rookSquare = src > dst ? dst + 1 : dst - 1;
		return getAttackBB(PIECETYPE_ROOK, rookSquare, getOccupancyBB() ^ square_bb[src]) & getPieceBB<PIECETYPE_KING>(them);
	}
	else if (isPromotion(move)) {
		PieceType promo = getPiecePromotion(move);
		return getAttackBB(promo, dst, getOccupancyBB() ^ square_bb[src]) &	getPieceBB<PIECETYPE_KING>(them);
	}
	
	return false;
}

int Position::see(Move move) const {
	Color color;
	Square src, dst, mayAttack;
	PieceType target;
	U64 attackers, from, occupancy, xRay, potential;
	int gain[32] = { 0 };
	int d = 0;

	color = us;
	src = getSrc(move);
	dst = getDst(move);
	from = square_bb[src];
	// Check if the move is en passant
	if (onSquare(src) == PIECETYPE_PAWN && square_bb[dst] & enPassant) {
		gain[d] = PIECE_VALUE[PIECETYPE_PAWN].score();
	}
	else if (getPiecePromotion(move) == PIECETYPE_QUEEN) {
		gain[d] = PIECE_VALUE[PIECETYPE_QUEEN].score() - PIECE_VALUE[PIECETYPE_PAWN].score();
	}
	else {
		gain[d] = PIECE_VALUE[onSquare(dst)].score();
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
		gain[d] = PIECE_VALUE[target].score() - gain[d - 1];

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
	fiftyMoveRule++;

	// Remove the ep file and castle rights from the zobrist key.
	if (enPassant) {
		key ^= Zobrist::key(get_file(enPassant));
	}
	key ^= Zobrist::key(castleRights);

	if (captured != PIECETYPE_NONE && !isCastle(move)) {
		fiftyMoveRule = 0;
		removePiece(them, captured, dst);
		if (captured == PIECETYPE_PAWN) {
			pawnKey ^= Zobrist::key(them, PIECETYPE_PAWN, dst);
		}
		gamePhase = true;
	}

	if (isCastle(move)) {
		// Short castle
		if (dst < src) {
			movePiece(us, PIECETYPE_ROOK, src-3, dst+1);
			movePiece(us, PIECETYPE_KING, src, dst);
		}
		// Long castle
		else {
			movePiece(us, PIECETYPE_ROOK, src+4, dst-1);
			movePiece(us, PIECETYPE_KING, src, dst);
		}
	}
	else {
		movePiece(us, moved, src, dst);
	}
	
	if (moved == PIECETYPE_PAWN) {
		fiftyMoveRule = 0;
		pawnKey ^= Zobrist::key(us, PIECETYPE_PAWN, src, dst);

		// Check for double PIECETYPE_PAWN push
		if (int(std::max(src, dst)) - int(std::min(src, dst)) == 16) {
			enPassant = us == WHITE ? square_bb[dst - 8] : square_bb[dst + 8];

			key ^= Zobrist::key(get_file(enPassant));
			epFlag = true;
		}
		else if (getPiecePromotion(move)) {
			pawnKey ^= Zobrist::key(us, PIECETYPE_PAWN, dst);
			removePiece(us, PIECETYPE_PAWN, dst);
			addPiece(us, getPiecePromotion(move), dst);
			gamePhase = true;
		}
		else if (enPassant & square_bb[dst]) {
			Square epCapture = (us == WHITE) ? dst - 8 : dst + 8;
			pawnKey ^= Zobrist::key(them, PIECETYPE_PAWN, epCapture);
			removePiece(them, PIECETYPE_PAWN, epCapture);
			gamePhase = true;
		}
	}

	if (!epFlag) {
		enPassant = 0;
	}
	if (gamePhase) {
		setGamePhase();
	}
	// Update castle rights
	castleRights &= CASTLE_RIGHTS[src];
	castleRights &= CASTLE_RIGHTS[dst];

	key ^= Zobrist::key(castleRights);

	assert(!check());
	assert((enPassant & getOccupancyBB()) == 0);
	previousMove = move;
	swapTurn();

	setPins(WHITE);
	setPins(BLACK);
	setCheckers();
}

void Position::makeNull() {
	assert(checkers == 0);
	// Remove the ep file and castle rights from the zobrist key
	if (enPassant) {
		key ^= Zobrist::key(get_file(enPassant));
	}
	// Reset the en-passant square
	enPassant = 0;

	// Swap the turn
	swapTurn();

	// Set check squares
	checkSquares[PIECETYPE_PAWN] = getAttackBB<PIECETYPE_PAWN>(getKingSquare(them), them);
	checkSquares[PIECETYPE_KNIGHT] = getAttackBB<PIECETYPE_KNIGHT>(getKingSquare(them));
	checkSquares[PIECETYPE_BISHOP] = getAttackBB<PIECETYPE_BISHOP>(getKingSquare(them));
	checkSquares[PIECETYPE_ROOK] = getAttackBB<PIECETYPE_ROOK>(getKingSquare(them));
	checkSquares[PIECETYPE_QUEEN] = checkSquares[PIECETYPE_BISHOP] | checkSquares[PIECETYPE_ROOK];
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
	if (us == WHITE) {
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
		for (Square p : getPieceList<PIECETYPE_PAWN>(us)) {
			if (rank(p) == rank(dst) && std::max(file(p), file(dst)) - std::min(file(p), file(dst)) == 1) {
				fen += SQUARE_TO_STRING[dst + (us == WHITE ? 8 : -8)];
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
std::ostream &operator<<(std::ostream &os, const Position &s) {
	std::string W_pawn   = "\u2659";
	std::string W_knight = "\u2658";
	std::string W_bishop = "\u2657";
	std::string W_rook   = "\u2656";
	std::string W_queen  = "\u2655";
	std::string W_king   = "\u2654";
	std::string B_pawn   = "\u265F";
	std::string B_knight = "\u265E";
	std::string B_bishop = "\u265D";
	std::string B_rook   = "\u265C";
	std::string B_queen  = "\u265B";
	std::string B_king   = "\u265A";
	std::string Empty    = " ";
	
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
	
	os << "Key:     " << s.key << '\n';
	os << "PawnKey: " << s.pawnKey << '\n';
	os << "Phase:   " << s.getGamePhase() << '\n';
	os << "previous move: " << to_string(s.previousMove) << '\n';
	if (s.us == WHITE) {
		os << "White to move\n";
	}
	else {
		os << "Black to move\n";
	}
	
	return os;
}