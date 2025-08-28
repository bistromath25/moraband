/**
 * Moraband, known in antiquity as Korriban, was an 
 * Outer Rim planet that was home to the ancient Sith 
 **/

#ifndef MOVE_GENERATOR_H
#define MOVE_GENERATOR_H

#include "MagicMoves.hpp"
#include "defs.h"
#include "history.h"
#include "move.h"
#include "position.h"
#include <array>
#include <vector>

constexpr U64 FULL = 0xFFFFFFFFFFFFFFFF;
constexpr int MOVELIST_MAX_SIZE = 256;

/**
 * Move generation stages for staged move generation
 * Used to prioritize move ordering and generation
 */
enum MoveStage {
    BestMove,
    AttacksGen,
    Attacks,
    Killer1,
    Killer2,
    QuietsGen,
    Quiets,
    QBestMove,
    QAttacksGen,
    QAttacks,
    QQuietChecksGen,
    QQuietChecks,
    EvadeBestMove,
    EvadeMovesGen,
    Evade,
    AllLegal
};

/** Types of moves for move generation */
enum class MoveType {
    Attacks,
    Quiets,
    Evasions,
    QuietChecks,
    All
};

/**
 * MoveList class for generating and managing moves
 * Implements staged move generation for efficient move ordering
 */
class MoveList {
public:
    MoveList(const Position &s, Move bestMove, History *history, int ply, bool qsearch = false);
    MoveList(const Position &s);
    std::size_t size() const;
    void push(Move m);
    bool contains(Move move) const;
    Move getBestMove();
    Move pop();
    void checkLegal();
    void setStage(int s) { stage = s; }

    template<MoveType T>
    void generateMoves();
    template<MoveType T, Color C>
    void pushPawnMoves();
    template<MoveType T, PieceType P>
    void pushMoves();
    template<MoveType T>
    void pushCastle();
    template<MoveType T>
    void pushPromotion(Square src, Square dst);
    friend std::ostream &operator<<(std::ostream &os, const MoveList &moveList);

private:
    bool isQSearch;
    U64 valid;
    U64 discover;
    const Position &position;
    const History *history;
    int ply;
    int stage;
    std::array<MoveEntry, MOVELIST_MAX_SIZE> moveList;
    std::vector<MoveEntry> badCaptures;
    std::size_t sz;
    Move best;
    Move killer1;
    Move killer2;
};

/** Initialize magic bitboard move generation */
void mg_init();

#endif
