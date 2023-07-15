/**
 * Moraband, known in antiquity as Korriban, was an 
 * Outer Rim planet that was home to the ancient Sith 
 **/

#ifndef POSITION_H
#define POSITION_H

#include <iostream>
#include <string>
#include <cmath>
#include <cstring>
#include <algorithm>
#include <array>
#include "pst.h"
#include "board.h"
#include "defs.h"
#include "move.h"
#include "zobrist.h"

/* Game phase calculation piece values */
enum Phase {
	pawnPhase   = 0,
	knightPhase = 1,
	bishopPhase = 1,
	rookPhase   = 2,
	queenPhase  = 4,
	totalPhase  = 24
};

/* Board position and related functions */
class Position {
public:
	Position() {};
	Position(const std::string & fen);
	Position(const Position & s);
	void operator=(const Position & s);
	void init();

	Color getOurColor() const;
	Color getTheirColor() const;
	int getFiftyMoveRule() const;
	U64 getKey() const;
	U64 getPawnKey() const;
	U64 getEnPassantBB() const;
	int getCastleRights() const;
	U64 getCheckersBB() const;
	PieceType onSquare(const Square s) const;
	Square getKingSquare(Color c) const;
	int getPstScore(GameStage g) const;
	int getGamePhase() const;
	void setGamePhase();
	std::string getFen();
	Move getPreviousMove() const; // Previous move made

	// Access piece bitboards
	template<PieceType P> const std::array<Square, PIECE_MAX>& getPieceList(Color c) const;
	template<PieceType P> U64 getPieceBB(Color c) const;
	template<PieceType P> U64 getPieceBB() const;
	template<PieceType P> int getPieceCount(Color c) const;
	template<PieceType P> int getPieceCount() const;
	int getNonPawnPieceCount(Color c) const;

	// Board occupancy
	U64 getOccupancyBB() const;
	U64 getOccupancyBB(Color c) const;
	U64 getEmptyBB() const;

	// Castle rights
	bool canCastleKingside() const;
	bool canCastleKingside(Color c) const;
	bool canCastleQueenside() const;
	bool canCastleQueenside(Color c) const;
	bool isQuiet(Move move) const;
	bool isCapture(Move move) const;
	bool isEnPassant(Move m) const;
	bool isValid(Move move, U64 validMoves) const;
	bool givesCheck(Move move) const;

	// Make move functions
	void makeMove(Move m);
	void makeNull();
	void addPiece(Color c, PieceType p, Square sq);
	void movePiece(Color c, PieceType p, Square src, Square dst);
	void removePiece(Color c, PieceType p, Square sq);
	void swapTurn();

	// Valid King moves and pins
	U64 getCheckSquaresBB(PieceType p) const;
	void setCheckers();
	void setPins(Color c);
	U64 getPinsBB(Color c) const;
	U64 getDiscoveredChecks(Color c) const;

	// Check and attack information
	bool isLegal(Move move) const;
	bool isAttacked(Square sq, Color c, U64 change) const;
	bool attacked(Square sq) const;
	bool defended(Square sq, Color c) const;
	bool check() const;
	bool check(U64 change) const;
	bool check(U64 change, Color c) const;
	bool inCheck() const;
	bool inDoubleCheck() const;
	bool insufficientMaterial() const;
	U64 getAttackersBB(Square sq, Color c) const;
	U64 allAttackers(Square sq) const;
	template<PieceType> U64 getAttackBB(Square sq, Color c=WHITE) const;
	U64 getAttackBB(PieceType p, Square sq, U64 occ) const;
	int see(Move m) const;
	U64 getXRayAttacks(Square sq) const;
	
	// Print
	friend std::ostream & operator << (std::ostream & os, const Position & Position);
	
private:
	Color mUs;
	Color mThem;
	int mFiftyMoveRule;
	int mCastleRights;
	int mPhase;
	U64 mKey;
	U64 mPawnKey;
	U64 mCheckers;
	U64 mEnPassant;
	Move mPreviousMove;
	std::array<U64, PIECE_TYPES_SIZE> mCheckSquares;
	std::array<U64, PLAYER_SIZE> mPinned;
	std::array<U64, PLAYER_SIZE> mOccupancy;
	std::array<int, BOARD_SIZE> mPieceIndex;
	std::array<PieceType, BOARD_SIZE> mBoard;
	std::array<std::array<U64, PIECE_TYPES_SIZE>, PLAYER_SIZE> mPieces;
	std::array<std::array<int, PIECE_TYPES_SIZE>, PLAYER_SIZE> mPieceCount;
	std::array<std::array<int, GAMESTAGE_SIZE>, PLAYER_SIZE> mPstScore;
	std::array<std::array<std::array<Square, PIECE_MAX>, PIECE_TYPES_SIZE>, PLAYER_SIZE> mPieceList;
};

inline Color Position::getOurColor() const {
	return mUs;
}

inline Color Position::getTheirColor() const {
	return mThem;
}

inline int Position::getFiftyMoveRule() const {
	return mFiftyMoveRule;
}

inline U64 Position::getKey() const {
	return mKey;
}

inline U64 Position::getPawnKey() const {
	return mPawnKey;
}

inline U64 Position::getEnPassantBB() const {
	return mEnPassant;
}

inline int Position::getCastleRights() const {
	return mCastleRights;
}

inline U64 Position::getPinsBB(Color c) const {
	return mPinned[c];
}

inline int Position::getGamePhase() const {
	return mPhase;
}

inline void Position::setGamePhase() {
	int phase = totalPhase
		- getPieceCount<PIECETYPE_PAWN>() * pawnPhase
			- getPieceCount<PIECETYPE_KNIGHT>() * knightPhase
				- getPieceCount<PIECETYPE_BISHOP>() * bishopPhase
					- getPieceCount<PIECETYPE_ROOK>() * rookPhase
						- getPieceCount<PIECETYPE_QUEEN>() * queenPhase;
	mPhase = (phase * 256 + (totalPhase / 2)) / totalPhase;
}

inline bool Position::isCapture(Move move) const {
	return square_bb[getDst(move)] & (getOccupancyBB(mThem) | mEnPassant);
}

inline bool Position::isQuiet(Move move) const {
	return !isCapture(move) && !isPromotion(move);
}

inline bool Position::isEnPassant(Move move) const {
	return onSquare(getSrc(move)) == PIECETYPE_PAWN && (square_bb[getDst(move)] & mEnPassant);
}

template<PieceType P> inline const std::array<Square, PIECE_MAX>& Position::getPieceList(Color c) const {
	return mPieceList[c][P];
}

inline void Position::addPiece(Color c, PieceType p, Square sq) {
	mPieces[c][p] |= square_bb[sq];
	mOccupancy[c] |= square_bb[sq];
	mBoard[sq] = p;
	mPieceIndex[sq] = mPieceCount[c][p]++;
	mPieceList[c][p][mPieceIndex[sq]] = sq;

	mPstScore[c][MIDDLEGAME] += PieceSquareTable::getScore(p, MIDDLEGAME, c, sq);
	mPstScore[c][ENDGAME] += PieceSquareTable::getScore(p, ENDGAME, c, sq);

	mKey ^= Zobrist::key(c, p, sq);
}

inline void Position::movePiece(Color c, PieceType p, Square src, Square dst) {
	mPieces[c][p] ^= square_bb[src] | square_bb[dst];
	mOccupancy[c] ^= square_bb[src] | square_bb[dst];
	mBoard[dst] = p;
	mBoard[src] = PIECETYPE_NONE;
	mPieceIndex[dst] = mPieceIndex[src];
	mPieceList[c][p][mPieceIndex[dst]] = dst;

	mPstScore[c][MIDDLEGAME] -= PieceSquareTable::getScore(p, MIDDLEGAME, c, src);
	mPstScore[c][ENDGAME] -= PieceSquareTable::getScore(p, ENDGAME, c, src);
	mPstScore[c][MIDDLEGAME] += PieceSquareTable::getScore(p, MIDDLEGAME, c, dst);
	mPstScore[c][ENDGAME] += PieceSquareTable::getScore(p, ENDGAME, c, dst);

	mKey ^= Zobrist::key(c, p, src, dst);
}

inline void Position::removePiece(Color c, PieceType p, Square sq) {
	Square swap;
	int pieceCount;

	mPieces[c][p] &= ~(square_bb[sq]);
	mOccupancy[c] &= ~(square_bb[sq]);
	mBoard[sq] = PIECETYPE_NONE;

	pieceCount = --mPieceCount[c][p];

	swap = mPieceList[c][p][pieceCount];
	mPieceIndex[swap] = mPieceIndex[sq];
	mPieceList[c][p][mPieceIndex[swap]] = swap;
	mPieceList[c][p][pieceCount] = no_sq;

	mPstScore[c][MIDDLEGAME] -= PieceSquareTable::getScore(p, MIDDLEGAME, c, sq);
	mPstScore[c][ENDGAME] -= PieceSquareTable::getScore(p, ENDGAME, c, sq);

	mKey ^= Zobrist::key(c, p, sq);
}

inline void Position::swapTurn() {
	mKey ^= Zobrist::key();
	mThem =  mUs;
	mUs   = !mUs;
}

template<PieceType P> inline U64 Position::getPieceBB(Color c) const {
	return mPieces[c][P];
}

template<PieceType P> inline U64 Position::getPieceBB() const {
	return mPieces[WHITE][P] | mPieces[BLACK][P];
}

template<PieceType P> inline int Position::getPieceCount() const {
	return mPieceCount[WHITE][P] + mPieceCount[BLACK][P];
}

template<PieceType P> inline int Position::getPieceCount(Color c) const {
	return mPieceCount[c][P];
}

inline int Position::getNonPawnPieceCount(Color c) const {
	return mPieceCount[c][PIECETYPE_KNIGHT] + mPieceCount[c][PIECETYPE_BISHOP] + mPieceCount[c][PIECETYPE_ROOK]   + mPieceCount[c][PIECETYPE_QUEEN];
}

inline Square Position::getKingSquare(Color c) const {
	return mPieceList[c][PIECETYPE_KING][0];
}

inline int Position::getPstScore(GameStage g) const {
	return mPstScore[mUs][g] - mPstScore[mThem][g];
}

inline PieceType Position::onSquare(const Square sq) const {
	return mBoard[sq];
}

inline U64 Position::getOccupancyBB() const {
	return mOccupancy[WHITE] | mOccupancy[BLACK];
}

inline U64 Position::getOccupancyBB(Color c) const {
	return mOccupancy[c];
}

inline U64 Position::getEmptyBB() const {
	return ~(mOccupancy[WHITE] | mOccupancy[BLACK]);
}

inline U64 Position::getCheckersBB() const {
	return mCheckers;
}

inline bool Position::canCastleKingside() const {
	return mUs ? mCastleRights & BLACK_KINGSIDE_CASTLE : mCastleRights & WHITE_KINGSIDE_CASTLE;
}

inline bool Position::canCastleKingside(Color c) const {
	return c == WHITE ? mCastleRights & WHITE_KINGSIDE_CASTLE : mCastleRights & BLACK_KINGSIDE_CASTLE;
}

inline bool Position::canCastleQueenside() const {
	return mUs ? mCastleRights & BLACK_QUEENSIDE_CASTLE : mCastleRights & WHITE_QUEENSIDE_CASTLE;
}

inline bool Position::canCastleQueenside(Color c) const {
	return c == WHITE ? mCastleRights & WHITE_QUEENSIDE_CASTLE : mCastleRights & BLACK_QUEENSIDE_CASTLE;
}

template<PieceType PIECETYPE_PAWN> inline U64 Position::getAttackBB(Square sq, Color c) const {
	return pawn_attacks[c][sq];
}

template<> inline U64 Position::getAttackBB<PIECETYPE_KNIGHT>(Square sq, Color c) const {
	return KNIGHT_MOVES[sq];
}

template<> inline U64 Position::getAttackBB<PIECETYPE_BISHOP>(Square sq, Color c) const {
	return Bmagic(sq, getOccupancyBB());
}

template<> inline U64 Position::getAttackBB<PIECETYPE_ROOK>(Square sq, Color c) const {
	return Rmagic(sq, getOccupancyBB());
}

template<> inline U64 Position::getAttackBB<PIECETYPE_QUEEN>(Square sq, Color c) const {
	return Qmagic(sq, getOccupancyBB());
}

template<> inline U64 Position::getAttackBB<PIECETYPE_KING>(Square sq, Color c) const {
	return KING_MOVES[sq];
}

inline U64 Position::getAttackBB(PieceType p, Square sq, U64 occ) const {
	assert(p != PIECETYPE_PAWN);
	assert(p != PIECETYPE_KING);
	return p == PIECETYPE_KNIGHT ? getAttackBB<PIECETYPE_KNIGHT>(sq) 
		: p == PIECETYPE_BISHOP ? Bmagic(sq, occ) 
			: p == PIECETYPE_ROOK ? Rmagic(sq, occ)
				: Qmagic(sq, occ);
}

inline U64 Position::getCheckSquaresBB(PieceType pPiece) const {
	return mCheckSquares[pPiece];
}

inline void Position::setCheckers() {
	mCheckers = getAttackersBB(getKingSquare(mUs), mThem);
	mCheckSquares[PIECETYPE_PAWN] = getAttackBB<PIECETYPE_PAWN>(getKingSquare(mThem), mThem);
	mCheckSquares[PIECETYPE_KNIGHT] = getAttackBB<PIECETYPE_KNIGHT>(getKingSquare(mThem));
	mCheckSquares[PIECETYPE_BISHOP] = getAttackBB<PIECETYPE_BISHOP>(getKingSquare(mThem));
	mCheckSquares[PIECETYPE_ROOK] = getAttackBB<PIECETYPE_ROOK>(getKingSquare(mThem));
	mCheckSquares[PIECETYPE_QUEEN] = mCheckSquares[PIECETYPE_BISHOP] | mCheckSquares[PIECETYPE_ROOK];
}

inline bool Position::defended(Square s, Color c) const {
	return getAttackBB<PIECETYPE_PAWN>(s, !c) & getPieceBB<PIECETYPE_PAWN>(c)
		|| getAttackBB<PIECETYPE_KNIGHT>(s) & getPieceBB<PIECETYPE_KNIGHT>(c)
			|| getAttackBB<PIECETYPE_BISHOP>(s) & (getPieceBB<PIECETYPE_BISHOP>(c) | getPieceBB<PIECETYPE_QUEEN>(c))
				|| getAttackBB<PIECETYPE_ROOK>(s) & (getPieceBB<PIECETYPE_ROOK>(c) | getPieceBB<PIECETYPE_QUEEN>(c));
}

inline bool Position::isAttacked(Square sq, Color c, U64 change) const {
	U64 occupancy = getOccupancyBB() ^ change;
	return getAttackBB<PIECETYPE_PAWN>(sq, c) & getPieceBB<PIECETYPE_PAWN>(!c)
		|| getAttackBB<PIECETYPE_KNIGHT>(sq) & getPieceBB<PIECETYPE_KNIGHT>(!c)
			|| Bmagic(sq, occupancy) & (getPieceBB<PIECETYPE_BISHOP>(!c) | getPieceBB<PIECETYPE_QUEEN>(!c))
				|| Rmagic(sq, occupancy) & (getPieceBB<PIECETYPE_ROOK>(!c) | getPieceBB<PIECETYPE_QUEEN>(!c))
					|| getAttackBB<PIECETYPE_KING>(sq) & getPieceBB<PIECETYPE_KING>(!c);
}

inline bool Position::attacked(Square sq) const {
	return getAttackBB< PIECETYPE_PAWN >(sq, mUs) &  getPieceBB< PIECETYPE_PAWN >(mThem)
		|| getAttackBB<PIECETYPE_KNIGHT>(sq) &  getPieceBB<PIECETYPE_KNIGHT>(mThem)
			|| getAttackBB<PIECETYPE_BISHOP>(sq) & (getPieceBB<PIECETYPE_BISHOP>(mThem) | getPieceBB<PIECETYPE_QUEEN>(mThem))
				|| getAttackBB<PIECETYPE_ROOK>(sq) & (getPieceBB<PIECETYPE_ROOK>(mThem) | getPieceBB<PIECETYPE_QUEEN>(mThem));
}

inline bool Position::inCheck() const {
	return getCheckersBB();
}

inline bool Position::inDoubleCheck() const {
	return pop_count(getCheckersBB()) == 2;
}

inline bool Position::check() const {
	return attacked(getKingSquare(mUs));
}

inline U64 Position::getAttackersBB(Square sq, Color c) const {
	return (getAttackBB<PIECETYPE_PAWN>(sq, !c) & getPieceBB< PIECETYPE_PAWN >(c))
		| (getAttackBB<PIECETYPE_KNIGHT>(sq) & getPieceBB<PIECETYPE_KNIGHT>(c))
			| (getAttackBB<PIECETYPE_BISHOP>(sq) & (getPieceBB<PIECETYPE_BISHOP>(c) | getPieceBB<PIECETYPE_QUEEN>(c)))
				| (getAttackBB<PIECETYPE_ROOK>(sq) & (getPieceBB<PIECETYPE_ROOK>(c) | getPieceBB<PIECETYPE_QUEEN>(c)));
}

inline U64 Position::allAttackers(Square sq) const {
	return (getAttackBB<PIECETYPE_PAWN>(sq, mUs) & getPieceBB<PIECETYPE_PAWN>(mThem))
		| (getAttackBB<PIECETYPE_PAWN>(sq, mThem) & getPieceBB<PIECETYPE_PAWN>(mUs))
			| (getAttackBB<PIECETYPE_KNIGHT>(sq) & getPieceBB<PIECETYPE_KNIGHT>())
				| (getAttackBB<PIECETYPE_BISHOP>(sq) & (getPieceBB<PIECETYPE_BISHOP>() | getPieceBB<PIECETYPE_QUEEN>()))
					| (getAttackBB<PIECETYPE_ROOK>(sq) & (getPieceBB<PIECETYPE_ROOK>() | getPieceBB<PIECETYPE_QUEEN>()))
						| (getAttackBB<PIECETYPE_KING>(sq) & getPieceBB<PIECETYPE_KING>());
}

inline bool Position::check(U64 change) const {
	return (Bmagic(getKingSquare(mUs), getOccupancyBB() ^ change) & (getPieceBB<PIECETYPE_BISHOP>(mThem) | getPieceBB<PIECETYPE_QUEEN>(mThem))) 
		|| (Rmagic(getKingSquare(mUs), getOccupancyBB() ^ change) & (getPieceBB< PIECETYPE_ROOK >(mThem) | getPieceBB<PIECETYPE_QUEEN>(mThem)));
}

inline bool Position::check(U64 change, Color c) const {
	return (Bmagic(getKingSquare(c), getOccupancyBB() ^ change) & (getPieceBB<PIECETYPE_BISHOP>(!c) | getPieceBB<PIECETYPE_QUEEN>(!c))) 
		|| (Rmagic(getKingSquare(c), getOccupancyBB() ^ change) & (getPieceBB< PIECETYPE_ROOK >(!c) | getPieceBB<PIECETYPE_QUEEN>(!c)));
}

inline U64 Position::getXRayAttacks(Square square) const {
	return bishopMoves[square] & (getPieceBB<PIECETYPE_BISHOP>() | getPieceBB<PIECETYPE_QUEEN>()) | rookMoves[square] & (getPieceBB<PIECETYPE_ROOK>() | getPieceBB<PIECETYPE_QUEEN>());
}

inline Move Position::getPreviousMove() const {
	return mPreviousMove;
}

#endif