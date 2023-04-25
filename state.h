#ifndef STATE_H
#define STATE_H

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

enum Phase {
	pawnPhase   = 0,
	knightPhase = 1,
	bishopPhase = 1,
	rookPhase   = 2,
	queenPhase  = 4,
	totalPhase  = 24
};

class State {
public:
	State() {};
	State(const std::string &);
	State(const State & s);
	void operator=(const State &);
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
	//void updateGameMoves(std::string pMove);
	//std::string getGameMoves() const;
	std::string getFen();
	Move getPreviousMove() const; // Previous move made

	// Access piece bitboards
	template<PieceType P> const std::array<Square, PIECE_MAX>& getPieceList(Color pColor) const;
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
	bool isQuiet(Move pMove) const;
	bool isCapture(Move pMove) const;
	bool isEnPassant(Move m) const;
	bool isValid(Move pMove, U64 pValid) const;
	bool givesCheck(Move pMove) const;

	// Make move functions
	void makeMove(Move m);
	void makeNull();
	void addPiece(Color pColor, PieceType pPiece, Square pSquare);
	void movePiece(Color pColor, PieceType pPiece, Square pSrc, Square pDst);
	void removePiece(Color pColor, PieceType pPiece, Square pSquare);
	void swapTurn();

	// Valid PIECETYPE_KING moves and pins
	U64 getCheckSquaresBB(PieceType pPiece) const;
	void setCheckers();
	void setPins(Color c);
	U64 getPinsBB(Color c) const;
	U64 getDiscoveredChecks(Color c) const;

	// Check and attack information
	bool isLegal(Move pMove) const;
	bool isAttacked(Square pSquare, Color pColor, U64 pChange) const;
	bool attacked(Square s) const;
	bool defended(Square s, Color c) const;
	bool check() const;
	bool check(U64 change) const;
	bool check(U64 change, Color c) const;
	bool inCheck() const;
	bool inDoubleCheck() const;
	bool insufficientMaterial() const;
	U64 getAttackersBB(Square s, Color c) const;
	U64 allAttackers(Square s) const;
	template<PieceType> U64 getAttackBB(Square s, Color c=WHITE) const;
	U64 getAttackBB(PieceType pPiece, Square pSquare, U64 pOcc) const;
	int see(Move m) const;
	U64 getXRayAttacks(Square square) const;
	
	// Print
	friend std::ostream & operator << (std::ostream & os, const State & state);
	
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

	//std::string mGameMoves;
};

inline Color State::getOurColor() const {
	return mUs;
}

inline Color State::getTheirColor() const {
	return mThem;
}

inline int State::getFiftyMoveRule() const {
	return mFiftyMoveRule;
}

inline U64 State::getKey() const {
	return mKey;
}

inline U64 State::getPawnKey() const {
	return mPawnKey;
}

inline U64 State::getEnPassantBB() const {
	return mEnPassant;
}

inline int State::getCastleRights() const {
	return mCastleRights;
}

inline U64 State::getPinsBB(Color c) const {
	return mPinned[c];
}

inline int State::getGamePhase() const {
	return mPhase;
}

inline void State::setGamePhase() {
	int phase = totalPhase
		- getPieceCount<PIECETYPE_PAWN>() * pawnPhase
			- getPieceCount<PIECETYPE_KNIGHT>() * knightPhase
				- getPieceCount<PIECETYPE_BISHOP>() * bishopPhase
					- getPieceCount<PIECETYPE_ROOK>() * rookPhase
						- getPieceCount<PIECETYPE_QUEEN>() * queenPhase;
	mPhase = (phase * 256 + (totalPhase / 2)) / totalPhase;
}

inline bool State::isCapture(Move pMove) const {
	return square_bb[getDst(pMove)] & (getOccupancyBB(mThem) | mEnPassant);
}

inline bool State::isQuiet(Move m) const {
	return !isCapture(m) && !isPromotion(m); // No promotion?
}

inline bool State::isEnPassant(Move m) const {
	return onSquare(getSrc(m)) == PIECETYPE_PAWN && (square_bb[getDst(m)] & mEnPassant);
}

template<PieceType P> inline const std::array<Square, PIECE_MAX>& State::getPieceList(Color pColor) const {
	return mPieceList[pColor][P];
}

inline void State::addPiece(Color pColor, PieceType pPiece, Square pSquare) {
	mPieces[pColor][pPiece] |= square_bb[pSquare];
	mOccupancy[pColor] |= square_bb[pSquare];
	mBoard[pSquare] = pPiece;
	mPieceIndex[pSquare] = mPieceCount[pColor][pPiece]++;
	mPieceList[pColor][pPiece][mPieceIndex[pSquare]] = pSquare;

	mPstScore[pColor][MIDDLEGAME] += PieceSquareTable::getScore(pPiece, MIDDLEGAME, pColor, pSquare);
	mPstScore[pColor][ENDGAME] += PieceSquareTable::getScore(pPiece, ENDGAME, pColor, pSquare);

	mKey ^= Zobrist::key(pColor, pPiece, pSquare);
}

inline void State::movePiece(Color pColor, PieceType pPiece, Square pSrc, Square pDst) {
	mPieces[pColor][pPiece] ^= square_bb[pSrc] | square_bb[pDst];
	mOccupancy[pColor] ^= square_bb[pSrc] | square_bb[pDst];
	mBoard[pDst] = pPiece;
	mBoard[pSrc] = PIECETYPE_NONE;
	mPieceIndex[pDst] = mPieceIndex[pSrc];
	mPieceList[pColor][pPiece][mPieceIndex[pDst]] = pDst;

	mPstScore[pColor][MIDDLEGAME] -= PieceSquareTable::getScore(pPiece, MIDDLEGAME, pColor, pSrc);
	mPstScore[pColor][ENDGAME] -= PieceSquareTable::getScore(pPiece, ENDGAME, pColor, pSrc);
	mPstScore[pColor][MIDDLEGAME] += PieceSquareTable::getScore(pPiece, MIDDLEGAME, pColor, pDst);
	mPstScore[pColor][ENDGAME] += PieceSquareTable::getScore(pPiece, ENDGAME, pColor, pDst);

	mKey ^= Zobrist::key(pColor, pPiece, pSrc, pDst);
}

inline void State::removePiece(Color pColor, PieceType pPiece, Square pSquare) {
	Square swap;
	int pieceCount;

	mPieces[pColor][pPiece] &= ~(square_bb[pSquare]);
	mOccupancy[pColor] &= ~(square_bb[pSquare]);
	mBoard[pSquare] = PIECETYPE_NONE;

	pieceCount = --mPieceCount[pColor][pPiece];

	swap = mPieceList[pColor][pPiece][pieceCount];
	mPieceIndex[swap] = mPieceIndex[pSquare];
	mPieceList[pColor][pPiece][mPieceIndex[swap]] = swap;
	mPieceList[pColor][pPiece][pieceCount] = no_sq;

	mPstScore[pColor][MIDDLEGAME] -= PieceSquareTable::getScore(pPiece, MIDDLEGAME, pColor, pSquare);
	mPstScore[pColor][ENDGAME] -= PieceSquareTable::getScore(pPiece, ENDGAME, pColor, pSquare);

	mKey ^= Zobrist::key(pColor, pPiece, pSquare);
}

inline void State::swapTurn() {
	mKey ^= Zobrist::key();
	mThem =  mUs;
	mUs   = !mUs;
}

template<PieceType P> inline U64 State::getPieceBB(Color c) const {
	return mPieces[c][P];
}

template<PieceType P> inline U64 State::getPieceBB() const {
	return mPieces[WHITE][P] | mPieces[BLACK][P];
}

template<PieceType P> inline int State::getPieceCount() const {
	return mPieceCount[WHITE][P] + mPieceCount[BLACK][P];
}

template<PieceType P> inline int State::getPieceCount(Color c) const {
	return mPieceCount[c][P];
}

inline int State::getNonPawnPieceCount(Color c) const {
	return mPieceCount[c][PIECETYPE_KNIGHT] + mPieceCount[c][PIECETYPE_BISHOP] + mPieceCount[c][PIECETYPE_ROOK]   + mPieceCount[c][PIECETYPE_QUEEN];
}

inline Square State::getKingSquare(Color c) const {
	return mPieceList[c][PIECETYPE_KING][0];
}

inline int State::getPstScore(GameStage g) const {
	return mPstScore[mUs][g] - mPstScore[mThem][g];
}

inline PieceType State::onSquare(const Square s) const {
	return mBoard[s];
}

inline U64 State::getOccupancyBB() const {
	return mOccupancy[WHITE] | mOccupancy[BLACK];
}

inline U64 State::getOccupancyBB(Color c) const {
	return mOccupancy[c];
}

inline U64 State::getEmptyBB() const {
	return ~(mOccupancy[WHITE] | mOccupancy[BLACK]);
}

inline U64 State::getCheckersBB() const {
	return mCheckers;
}

inline bool State::canCastleKingside() const {
	return mUs ? mCastleRights & BLACK_KINGSIDE_CASTLE : mCastleRights & WHITE_KINGSIDE_CASTLE;
}

inline bool State::canCastleKingside(Color c) const {
	return c == WHITE ? mCastleRights & WHITE_KINGSIDE_CASTLE : mCastleRights & BLACK_KINGSIDE_CASTLE;
}

inline bool State::canCastleQueenside() const {
	return mUs ? mCastleRights & BLACK_QUEENSIDE_CASTLE : mCastleRights & WHITE_QUEENSIDE_CASTLE;
}

inline bool State::canCastleQueenside(Color c) const {
	return c == WHITE ? mCastleRights & WHITE_QUEENSIDE_CASTLE : mCastleRights & BLACK_QUEENSIDE_CASTLE;
}

template<PieceType PIECETYPE_PAWN> inline U64 State::getAttackBB(Square s, Color c) const {
	return pawn_attacks[c][s];
}

template<> inline U64 State::getAttackBB<PIECETYPE_KNIGHT>(Square s, Color c) const {
	return KNIGHT_MOVES[s];
}

template<> inline U64 State::getAttackBB<PIECETYPE_BISHOP>(Square s, Color c) const {
	return Bmagic(s, getOccupancyBB());
}

template<> inline U64 State::getAttackBB<PIECETYPE_ROOK>(Square s, Color c) const {
	return Rmagic(s, getOccupancyBB());
}

template<> inline U64 State::getAttackBB<PIECETYPE_QUEEN>(Square s, Color c) const {
	return Qmagic(s, getOccupancyBB());
}

template<> inline U64 State::getAttackBB<PIECETYPE_KING>(Square s, Color c) const {
	return KING_MOVES[s];
}

inline U64 State::getAttackBB(PieceType pPiece, Square pSquare, U64 pOcc) const {
	assert(pPiece != PIECETYPE_PAWN);
	assert(pPiece != PIECETYPE_KING);
	return pPiece == PIECETYPE_KNIGHT ? getAttackBB<PIECETYPE_KNIGHT>(pSquare) 
		: pPiece == PIECETYPE_BISHOP ? Bmagic(pSquare, pOcc) 
			: pPiece == PIECETYPE_ROOK ? Rmagic(pSquare, pOcc)
				: Qmagic(pSquare, pOcc);
}

inline U64 State::getCheckSquaresBB(PieceType pPiece) const {
	return mCheckSquares[pPiece];
}

inline void State::setCheckers() {
	mCheckers = getAttackersBB(getKingSquare(mUs), mThem);
	mCheckSquares[PIECETYPE_PAWN] = getAttackBB<PIECETYPE_PAWN>(getKingSquare(mThem), mThem);
	mCheckSquares[PIECETYPE_KNIGHT] = getAttackBB<PIECETYPE_KNIGHT>(getKingSquare(mThem));
	mCheckSquares[PIECETYPE_BISHOP] = getAttackBB<PIECETYPE_BISHOP>(getKingSquare(mThem));
	mCheckSquares[PIECETYPE_ROOK] = getAttackBB<PIECETYPE_ROOK>(getKingSquare(mThem));
	mCheckSquares[PIECETYPE_QUEEN] = mCheckSquares[PIECETYPE_BISHOP] | mCheckSquares[PIECETYPE_ROOK];
}

inline bool State::defended(Square s, Color c) const {
	return getAttackBB<PIECETYPE_PAWN>(s, !c) & getPieceBB<PIECETYPE_PAWN>(c)
		|| getAttackBB<PIECETYPE_KNIGHT>(s) & getPieceBB<PIECETYPE_KNIGHT>(c)
			|| getAttackBB<PIECETYPE_BISHOP>(s) & (getPieceBB<PIECETYPE_BISHOP>(c) | getPieceBB<PIECETYPE_QUEEN>(c))
				|| getAttackBB<PIECETYPE_ROOK>(s) & (getPieceBB<PIECETYPE_ROOK>(c) | getPieceBB<PIECETYPE_QUEEN>(c));
}

inline bool State::isAttacked(Square pSquare, Color pColor, U64 pChange) const {
	U64 occupancy = getOccupancyBB() ^ pChange;
	return getAttackBB<PIECETYPE_PAWN>(pSquare, pColor) & getPieceBB<PIECETYPE_PAWN>(!pColor)
		|| getAttackBB<PIECETYPE_KNIGHT>(pSquare) & getPieceBB<PIECETYPE_KNIGHT>(!pColor)
			|| Bmagic(pSquare, occupancy) & (getPieceBB<PIECETYPE_BISHOP>(!pColor) | getPieceBB<PIECETYPE_QUEEN>(!pColor))
				|| Rmagic(pSquare, occupancy) & (getPieceBB<PIECETYPE_ROOK>(!pColor) | getPieceBB<PIECETYPE_QUEEN>(!pColor))
					|| getAttackBB<PIECETYPE_KING>(pSquare) & getPieceBB<PIECETYPE_KING>(!pColor);
}

inline bool State::attacked(Square s) const {
	return getAttackBB< PIECETYPE_PAWN >(s, mUs) &  getPieceBB< PIECETYPE_PAWN >(mThem)
		|| getAttackBB<PIECETYPE_KNIGHT>(s) &  getPieceBB<PIECETYPE_KNIGHT>(mThem)
			|| getAttackBB<PIECETYPE_BISHOP>(s) & (getPieceBB<PIECETYPE_BISHOP>(mThem) | getPieceBB<PIECETYPE_QUEEN>(mThem))
				|| getAttackBB<PIECETYPE_ROOK>(s) & (getPieceBB<PIECETYPE_ROOK>(mThem) | getPieceBB<PIECETYPE_QUEEN>(mThem));
}

inline bool State::inCheck() const {
	return getCheckersBB();
}

inline bool State::inDoubleCheck() const {
	return pop_count(getCheckersBB()) == 2;
}

inline bool State::check() const {
	return attacked(getKingSquare(mUs));
}

inline U64 State::getAttackersBB(Square s, Color c) const {
	return (getAttackBB<PIECETYPE_PAWN>(s, !c) & getPieceBB< PIECETYPE_PAWN >(c))
		| (getAttackBB<PIECETYPE_KNIGHT>(s) & getPieceBB<PIECETYPE_KNIGHT>(c))
			| (getAttackBB<PIECETYPE_BISHOP>(s) & (getPieceBB<PIECETYPE_BISHOP>(c) | getPieceBB<PIECETYPE_QUEEN>(c)))
				| (getAttackBB<PIECETYPE_ROOK>(s) & (getPieceBB<PIECETYPE_ROOK>(c) | getPieceBB<PIECETYPE_QUEEN>(c)));
}

inline U64 State::allAttackers(Square s) const {
	return (getAttackBB<PIECETYPE_PAWN>(s, mUs) & getPieceBB<PIECETYPE_PAWN>(mThem))
		| (getAttackBB<PIECETYPE_PAWN>(s, mThem) & getPieceBB<PIECETYPE_PAWN>(mUs))
			| (getAttackBB<PIECETYPE_KNIGHT>(s) & getPieceBB<PIECETYPE_KNIGHT>())
				| (getAttackBB<PIECETYPE_BISHOP>(s) & (getPieceBB<PIECETYPE_BISHOP>() | getPieceBB<PIECETYPE_QUEEN>()))
					| (getAttackBB<PIECETYPE_ROOK>(s) & (getPieceBB<PIECETYPE_ROOK>() | getPieceBB<PIECETYPE_QUEEN>()))
						| (getAttackBB<PIECETYPE_KING>(s) & getPieceBB<PIECETYPE_KING>());
}

inline bool State::check(U64 change) const {
	return (Bmagic(getKingSquare(mUs), getOccupancyBB() ^ change) & (getPieceBB<PIECETYPE_BISHOP>(mThem) | getPieceBB<PIECETYPE_QUEEN>(mThem))) 
		|| (Rmagic(getKingSquare(mUs), getOccupancyBB() ^ change) & (getPieceBB< PIECETYPE_ROOK >(mThem) | getPieceBB<PIECETYPE_QUEEN>(mThem)));
}

inline bool State::check(U64 change, Color c) const {
	return (Bmagic(getKingSquare(c), getOccupancyBB() ^ change) & (getPieceBB<PIECETYPE_BISHOP>(!c) | getPieceBB<PIECETYPE_QUEEN>(!c))) 
		|| (Rmagic(getKingSquare(c), getOccupancyBB() ^ change) & (getPieceBB< PIECETYPE_ROOK >(!c) | getPieceBB<PIECETYPE_QUEEN>(!c)));
}

inline U64 State::getXRayAttacks(Square square) const {
	return bishopMoves[square] & (getPieceBB<PIECETYPE_BISHOP>() | getPieceBB<PIECETYPE_QUEEN>()) | rookMoves[square] & (getPieceBB<PIECETYPE_ROOK>() | getPieceBB<PIECETYPE_QUEEN>());
}

/*
inline void State::updateGameMoves(std::string pMove) {
	mGameMoves += pMove + ' ';
}

inline std::string State::getGameMoves() const {
	return mGameMoves;
}
*/

inline Move State::getPreviousMove() const {
	return mPreviousMove;
}

#endif

///