#ifndef TEST_UTILS_H
#define TEST_UTILS_H

#include "move.h"
#include "position.h"
#include <set>
#include <string>
#include <vector>

class TestUtils {
public:
    static void initialize();
    static std::vector<Move> getAllMoves(const Position &pos);
    static std::set<std::string> getAllMovesAsStrings(const Position &pos);
    static bool containsMove(const Position &pos, const std::string &moveStr);
    static int countCaptureMoves(const Position &pos);
    static int countQuietMoves(const Position &pos);
};

#endif
