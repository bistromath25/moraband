/**
 * Moraband, known in antiquity as Korriban, was an 
 * Outer Rim planet that was home to the ancient Sith 
 **/

#ifndef POSITION_H
#define POSITION_H

#include "board.h"
#include "defs.h"
#include "move.h"
#include "pst.h"
#include "zobrist.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <cstring>
#include <iostream>
#include <string>

/** Game phase calculation piece values */
enum Phase {
    pawnPhase = 0,
    knightPhase = 1,
    bishopPhase = 1,
    rookPhase = 2,
    queenPhase = 4,
    totalPhase = 24
};

/** Board position and related functions */
class Position {
public:
    Position();
    Position(const std::string &fen);
    Position(const Position &s);
    Position &operator=(const Position &s);
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
    Move getPreviousMove() const;

    template<PieceType P>
    const std::array<Square, PIECE_MAX> &getPieceList(Color c) const;
    template<PieceType P>
    U64 getPieceBB(Color c) const;
    template<PieceType P>
    U64 getPieceBB() const;
    template<PieceType P>
    int getPieceCount(Color c) const;
    template<PieceType P>
    int getPieceCount() const;
    int getNonPawnPieceCount(Color c) const;
    int getNonPawnPieceCount() const;

    // Get board occupancy
    U64 getOccupancyBB() const;
    U64 getOccupancyBB(Color c) const;
    U64 getEmptyBB() const;

    bool canCastleKingside() const;
    bool canCastleKingside(Color c) const;
    bool canCastleQueenside() const;
    bool canCastleQueenside(Color c) const;
    bool isQuiet(Move move) const;
    bool isCapture(Move move) const;
    bool isEnPassant(Move m) const;
    bool isValid(Move move, U64 validMoves) const;
    bool givesCheck(Move move) const;

    void makeMove(Move m);
    void makeNull();
    void addPiece(Color c, PieceType p, Square sq);
    void movePiece(Color c, PieceType p, Square src, Square dst);
    void removePiece(Color c, PieceType p, Square sq);
    void swapTurn();

    // Validate king moves and pins
    U64 getCheckSquaresBB(PieceType p) const;
    void setCheckers();
    void setPins(Color c);
    U64 getPinsBB(Color c) const;
    U64 getDiscoveredChecks(Color c) const;

    // Get checks and attacks
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
    template<PieceType>
    U64 getAttackBB(Square sq, Color c = WHITE) const;
    U64 getAttackBB(PieceType p, Square sq, U64 occ) const;
    int see(Move m) const;
    U64 getXRayAttacks(Square sq) const;

    friend std::ostream &operator<<(std::ostream &os, const Position &s);

private:
    Color us;
    Color them;
    int fiftyMoveRule;
    int castleRights;
    int phase;
    U64 key;
    U64 pawnKey;
    U64 checkers;
    U64 enPassant;
    Move previousMove;
    std::array<U64, PIECE_TYPES_SIZE> checkSquares;
    std::array<U64, PLAYER_SIZE> pinned;
    std::array<U64, PLAYER_SIZE> occupancy;
    std::array<int, BOARD_SIZE> pieceIndex;
    std::array<PieceType, BOARD_SIZE> board;
    std::array<std::array<U64, PIECE_TYPES_SIZE>, PLAYER_SIZE> pieces;
    std::array<std::array<int, PIECE_TYPES_SIZE>, PLAYER_SIZE> pieceCounts;
    std::array<std::array<int, GAMESTAGE_SIZE>, PLAYER_SIZE> pstScore;
    std::array<std::array<std::array<Square, PIECE_MAX>, PIECE_TYPES_SIZE>, PLAYER_SIZE> pieceList;
};

inline Color Position::getOurColor() const {
    return us;
}

inline Color Position::getTheirColor() const {
    return them;
}

inline int Position::getFiftyMoveRule() const {
    return fiftyMoveRule;
}

inline U64 Position::getKey() const {
    return key;
}

inline U64 Position::getPawnKey() const {
    return pawnKey;
}

inline U64 Position::getEnPassantBB() const {
    return enPassant;
}

inline int Position::getCastleRights() const {
    return castleRights;
}

inline U64 Position::getPinsBB(Color c) const {
    return pinned[c];
}

inline int Position::getGamePhase() const {
    return phase;
}

inline void Position::setGamePhase() {
    int p = totalPhase - getPieceCount<PIECETYPE_PAWN>() * pawnPhase - getPieceCount<PIECETYPE_KNIGHT>() * knightPhase - getPieceCount<PIECETYPE_BISHOP>() * bishopPhase - getPieceCount<PIECETYPE_ROOK>() * rookPhase - getPieceCount<PIECETYPE_QUEEN>() * queenPhase;
    phase = (p * 256 + (totalPhase / 2)) / totalPhase;
}

inline bool Position::isCapture(Move move) const {
    return square_bb[getDst(move)] & (getOccupancyBB(them) | enPassant);
}

inline bool Position::isQuiet(Move move) const {
    return !isCapture(move) && !isPromotion(move);
}

inline bool Position::isEnPassant(Move move) const {
    return onSquare(getSrc(move)) == PIECETYPE_PAWN && (square_bb[getDst(move)] & enPassant);
}

template<PieceType P>
inline const std::array<Square, PIECE_MAX> &Position::getPieceList(Color c) const {
    return pieceList[c][P];
}

inline void Position::addPiece(Color c, PieceType p, Square sq) {
    pieces[c][p] |= square_bb[sq];
    occupancy[c] |= square_bb[sq];
    board[sq] = p;
    pieceIndex[sq] = pieceCounts[c][p]++;
    pieceList[c][p][pieceIndex[sq]] = sq;

    pstScore[c][MIDDLEGAME] += PieceSquareTable::getScore(p, MIDDLEGAME, c, sq);
    pstScore[c][ENDGAME] += PieceSquareTable::getScore(p, ENDGAME, c, sq);

    key ^= Zobrist::key(c, p, sq);
}

inline void Position::movePiece(Color c, PieceType p, Square src, Square dst) {
    pieces[c][p] ^= square_bb[src] | square_bb[dst];
    occupancy[c] ^= square_bb[src] | square_bb[dst];
    board[dst] = p;
    board[src] = PIECETYPE_NONE;
    pieceIndex[dst] = pieceIndex[src];
    pieceList[c][p][pieceIndex[dst]] = dst;

    pstScore[c][MIDDLEGAME] -= PieceSquareTable::getScore(p, MIDDLEGAME, c, src);
    pstScore[c][ENDGAME] -= PieceSquareTable::getScore(p, ENDGAME, c, src);
    pstScore[c][MIDDLEGAME] += PieceSquareTable::getScore(p, MIDDLEGAME, c, dst);
    pstScore[c][ENDGAME] += PieceSquareTable::getScore(p, ENDGAME, c, dst);

    key ^= Zobrist::key(c, p, src, dst);
}

inline void Position::removePiece(Color c, PieceType p, Square sq) {
    Square swap;
    int pieceCount;

    pieces[c][p] &= ~(square_bb[sq]);
    occupancy[c] &= ~(square_bb[sq]);
    board[sq] = PIECETYPE_NONE;

    pieceCount = --pieceCounts[c][p];

    swap = pieceList[c][p][pieceCount];
    pieceIndex[swap] = pieceIndex[sq];
    pieceList[c][p][pieceIndex[swap]] = swap;
    pieceList[c][p][pieceCount] = no_sq;

    pstScore[c][MIDDLEGAME] -= PieceSquareTable::getScore(p, MIDDLEGAME, c, sq);
    pstScore[c][ENDGAME] -= PieceSquareTable::getScore(p, ENDGAME, c, sq);

    key ^= Zobrist::key(c, p, sq);
}

inline void Position::swapTurn() {
    key ^= Zobrist::key();
    them = us;
    us = !us;
}

template<PieceType P>
inline U64 Position::getPieceBB(Color c) const {
    return pieces[c][P];
}

template<PieceType P>
inline U64 Position::getPieceBB() const {
    return pieces[WHITE][P] | pieces[BLACK][P];
}

template<PieceType P>
inline int Position::getPieceCount() const {
    return pieceCounts[WHITE][P] + pieceCounts[BLACK][P];
}

template<PieceType P>
inline int Position::getPieceCount(Color c) const {
    return pieceCounts[c][P];
}

inline int Position::getNonPawnPieceCount(Color c) const {
    return pieceCounts[c][PIECETYPE_KNIGHT] + pieceCounts[c][PIECETYPE_BISHOP] + pieceCounts[c][PIECETYPE_ROOK] + pieceCounts[c][PIECETYPE_QUEEN];
}

inline int Position::getNonPawnPieceCount() const {
    return getNonPawnPieceCount(WHITE) + getNonPawnPieceCount(BLACK);
}

inline Square Position::getKingSquare(Color c) const {
    return pieceList[c][PIECETYPE_KING][0];
}

inline int Position::getPstScore(GameStage g) const {
    return pstScore[us][g] - pstScore[them][g];
}

inline PieceType Position::onSquare(const Square sq) const {
    return board[sq];
}

inline U64 Position::getOccupancyBB() const {
    return occupancy[WHITE] | occupancy[BLACK];
}

inline U64 Position::getOccupancyBB(Color c) const {
    return occupancy[c];
}

inline U64 Position::getEmptyBB() const {
    return ~(occupancy[WHITE] | occupancy[BLACK]);
}

inline U64 Position::getCheckersBB() const {
    return checkers;
}

inline bool Position::canCastleKingside() const {
    return us ? castleRights & BLACK_KINGSIDE_CASTLE : castleRights & WHITE_KINGSIDE_CASTLE;
}

inline bool Position::canCastleKingside(Color c) const {
    return c == WHITE ? castleRights & WHITE_KINGSIDE_CASTLE : castleRights & BLACK_KINGSIDE_CASTLE;
}

inline bool Position::canCastleQueenside() const {
    return us ? castleRights & BLACK_QUEENSIDE_CASTLE : castleRights & WHITE_QUEENSIDE_CASTLE;
}

inline bool Position::canCastleQueenside(Color c) const {
    return c == WHITE ? castleRights & WHITE_QUEENSIDE_CASTLE : castleRights & BLACK_QUEENSIDE_CASTLE;
}

template<PieceType PIECETYPE_PAWN>
inline U64 Position::getAttackBB(Square sq, Color c) const {
    return pawn_attacks[c][sq];
}

template<>
inline U64 Position::getAttackBB<PIECETYPE_KNIGHT>(Square sq, Color c) const {
    return KNIGHT_MOVES[sq];
}

template<>
inline U64 Position::getAttackBB<PIECETYPE_BISHOP>(Square sq, Color c) const {
    return Bmagic(sq, getOccupancyBB());
}

template<>
inline U64 Position::getAttackBB<PIECETYPE_ROOK>(Square sq, Color c) const {
    return Rmagic(sq, getOccupancyBB());
}

template<>
inline U64 Position::getAttackBB<PIECETYPE_QUEEN>(Square sq, Color c) const {
    return Qmagic(sq, getOccupancyBB());
}

template<>
inline U64 Position::getAttackBB<PIECETYPE_KING>(Square sq, Color c) const {
    return KING_MOVES[sq];
}

inline U64 Position::getAttackBB(PieceType p, Square sq, U64 occ) const {
    assert(p != PIECETYPE_PAWN);
    assert(p != PIECETYPE_KING);
    return p == PIECETYPE_KNIGHT   ? getAttackBB<PIECETYPE_KNIGHT>(sq)
           : p == PIECETYPE_BISHOP ? Bmagic(sq, occ)
           : p == PIECETYPE_ROOK   ? Rmagic(sq, occ)
                                   : Qmagic(sq, occ);
}

inline U64 Position::getCheckSquaresBB(PieceType pPiece) const {
    return checkSquares[pPiece];
}

inline void Position::setCheckers() {
    checkers = getAttackersBB(getKingSquare(us), them);
    checkSquares[PIECETYPE_PAWN] = getAttackBB<PIECETYPE_PAWN>(getKingSquare(them), them);
    checkSquares[PIECETYPE_KNIGHT] = getAttackBB<PIECETYPE_KNIGHT>(getKingSquare(them));
    checkSquares[PIECETYPE_BISHOP] = getAttackBB<PIECETYPE_BISHOP>(getKingSquare(them));
    checkSquares[PIECETYPE_ROOK] = getAttackBB<PIECETYPE_ROOK>(getKingSquare(them));
    checkSquares[PIECETYPE_QUEEN] = checkSquares[PIECETYPE_BISHOP] | checkSquares[PIECETYPE_ROOK];
}

inline bool Position::defended(Square s, Color c) const {
    return getAttackBB<PIECETYPE_PAWN>(s, !c) & getPieceBB<PIECETYPE_PAWN>(c) || getAttackBB<PIECETYPE_KNIGHT>(s) & getPieceBB<PIECETYPE_KNIGHT>(c) || getAttackBB<PIECETYPE_BISHOP>(s) & (getPieceBB<PIECETYPE_BISHOP>(c) | getPieceBB<PIECETYPE_QUEEN>(c)) || getAttackBB<PIECETYPE_ROOK>(s) & (getPieceBB<PIECETYPE_ROOK>(c) | getPieceBB<PIECETYPE_QUEEN>(c));
}

inline bool Position::isAttacked(Square sq, Color c, U64 change) const {
    U64 occupancy = getOccupancyBB() ^ change;
    return getAttackBB<PIECETYPE_PAWN>(sq, c) & getPieceBB<PIECETYPE_PAWN>(!c) || getAttackBB<PIECETYPE_KNIGHT>(sq) & getPieceBB<PIECETYPE_KNIGHT>(!c) || Bmagic(sq, occupancy) & (getPieceBB<PIECETYPE_BISHOP>(!c) | getPieceBB<PIECETYPE_QUEEN>(!c)) || Rmagic(sq, occupancy) & (getPieceBB<PIECETYPE_ROOK>(!c) | getPieceBB<PIECETYPE_QUEEN>(!c)) || getAttackBB<PIECETYPE_KING>(sq) & getPieceBB<PIECETYPE_KING>(!c);
}

inline bool Position::attacked(Square sq) const {
    return getAttackBB<PIECETYPE_PAWN>(sq, us) & getPieceBB<PIECETYPE_PAWN>(them) || getAttackBB<PIECETYPE_KNIGHT>(sq) & getPieceBB<PIECETYPE_KNIGHT>(them) || getAttackBB<PIECETYPE_BISHOP>(sq) & (getPieceBB<PIECETYPE_BISHOP>(them) | getPieceBB<PIECETYPE_QUEEN>(them)) || getAttackBB<PIECETYPE_ROOK>(sq) & (getPieceBB<PIECETYPE_ROOK>(them) | getPieceBB<PIECETYPE_QUEEN>(them));
}

inline bool Position::inCheck() const {
    return getCheckersBB();
}

inline bool Position::inDoubleCheck() const {
    return pop_count(getCheckersBB()) == 2;
}

inline bool Position::check() const {
    return attacked(getKingSquare(us));
}

inline U64 Position::getAttackersBB(Square sq, Color c) const {
    return (getAttackBB<PIECETYPE_PAWN>(sq, !c) & getPieceBB<PIECETYPE_PAWN>(c)) | (getAttackBB<PIECETYPE_KNIGHT>(sq) & getPieceBB<PIECETYPE_KNIGHT>(c)) | (getAttackBB<PIECETYPE_BISHOP>(sq) & (getPieceBB<PIECETYPE_BISHOP>(c) | getPieceBB<PIECETYPE_QUEEN>(c))) | (getAttackBB<PIECETYPE_ROOK>(sq) & (getPieceBB<PIECETYPE_ROOK>(c) | getPieceBB<PIECETYPE_QUEEN>(c)));
}

inline U64 Position::allAttackers(Square sq) const {
    return (getAttackBB<PIECETYPE_PAWN>(sq, us) & getPieceBB<PIECETYPE_PAWN>(them)) | (getAttackBB<PIECETYPE_PAWN>(sq, them) & getPieceBB<PIECETYPE_PAWN>(us)) | (getAttackBB<PIECETYPE_KNIGHT>(sq) & getPieceBB<PIECETYPE_KNIGHT>()) | (getAttackBB<PIECETYPE_BISHOP>(sq) & (getPieceBB<PIECETYPE_BISHOP>() | getPieceBB<PIECETYPE_QUEEN>())) | (getAttackBB<PIECETYPE_ROOK>(sq) & (getPieceBB<PIECETYPE_ROOK>() | getPieceBB<PIECETYPE_QUEEN>())) | (getAttackBB<PIECETYPE_KING>(sq) & getPieceBB<PIECETYPE_KING>());
}

inline bool Position::check(U64 change) const {
    return (Bmagic(getKingSquare(us), getOccupancyBB() ^ change) & (getPieceBB<PIECETYPE_BISHOP>(them) | getPieceBB<PIECETYPE_QUEEN>(them))) || (Rmagic(getKingSquare(us), getOccupancyBB() ^ change) & (getPieceBB<PIECETYPE_ROOK>(them) | getPieceBB<PIECETYPE_QUEEN>(them)));
}

inline bool Position::check(U64 change, Color c) const {
    return (Bmagic(getKingSquare(c), getOccupancyBB() ^ change) & (getPieceBB<PIECETYPE_BISHOP>(!c) | getPieceBB<PIECETYPE_QUEEN>(!c))) || (Rmagic(getKingSquare(c), getOccupancyBB() ^ change) & (getPieceBB<PIECETYPE_ROOK>(!c) | getPieceBB<PIECETYPE_QUEEN>(!c)));
}

inline U64 Position::getXRayAttacks(Square square) const {
    return (bishopMoves[square] & ((getPieceBB<PIECETYPE_BISHOP>() | getPieceBB<PIECETYPE_QUEEN>()))) | (rookMoves[square] & ((getPieceBB<PIECETYPE_ROOK>() | getPieceBB<PIECETYPE_QUEEN>())));
}

inline Move Position::getPreviousMove() const {
    return previousMove;
}

#endif
