#include "test_utils.h"
#include "board.h"
#include "eval.h"
#include "movegen.h"
#include "search.h"
#include "zobrist.h"
#include <gtest/gtest.h>

void TestUtils::initialize() {
    static bool initialized = false;
    if (!initialized) {
        mg_init();
        Zobrist::init();
        bb_init();
        initKingRing();
        initialized = true;
    }
}

std::vector<Move> TestUtils::getAllMoves(const Position &pos) {
    MoveList moveList(pos);
    std::vector<Move> moves;

    while (moveList.size() > 0) {
        moves.push_back(moveList.pop());
    }

    return moves;
}

std::set<std::string> TestUtils::getAllMovesAsStrings(const Position &pos) {
    MoveList moveList(pos);
    std::set<std::string> moves;

    while (moveList.size() > 0) {
        moves.insert(to_string(moveList.pop()));
    }

    return moves;
}

bool TestUtils::containsMove(const Position &pos, const std::string &moveStr) {
    MoveList moveList(pos);

    while (moveList.size() > 0) {
        if (to_string(moveList.pop()) == moveStr) {
            return true;
        }
    }

    return false;
}

int TestUtils::countCaptureMoves(const Position &pos) {
    MoveList moveList(pos);
    int count = 0;

    while (moveList.size() > 0) {
        Move move = moveList.pop();
        if (pos.isCapture(move)) {
            ++count;
        }
    }

    return count;
}

int TestUtils::countQuietMoves(const Position &pos) {
    MoveList moveList(pos);
    int count = 0;

    while (moveList.size() > 0) {
        Move move = moveList.pop();
        if (pos.isQuiet(move)) {
            ++count;
        }
    }

    return count;
}
