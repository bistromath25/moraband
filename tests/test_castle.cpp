#include "move.h"
#include "movegen.h"
#include "position.h"
#include "test_utils.h"
#include <gtest/gtest.h>
#include <string>
#include <vector>

struct CastleTestPosition {
    std::string fen;
    std::vector<std::string> findMoves;
    std::vector<std::string> avoidMoves;
};

const std::vector<CastleTestPosition> STANDARD_CASTLE_POSITIONS = {
    {"r3k2r/8/8/8/8/8/8/R3K2R w KQkq - 0 1", {"e1g1", "e1c1"}, {}},
    {"r3k2r/8/8/8/8/8/8/R3K2R b KQkq - 0 1", {"e8g8", "e8c8"}, {}},
    {"r3k2r/4B3/8/8/8/8/4b3/R3K2R w KQkq - 0 1", {}, {"e1g1", "e1c1"}},
    {"r3k2r/4B3/8/8/8/8/4b3/R3K2R b KQkq - 0 1", {}, {"e8g8", "e8c8"}},
    {"r1B1kb1r/8/8/8/8/8/8/R1b1KB1R w KQkq - 0 1", {}, {"e1g1", "e1c1"}},
    {"r1B1kb1r/8/8/8/8/8/8/R1b1KB1R b KQkq - 0 1", {}, {"e8g8", "e8c8"}},
    {"r3k2r/8/5B2/8/8/8/8/Rb2K2R w KQkq - 0 1", {"e1g1"}, {"e1c1"}},
    {"r3k2r/6B1/8/8/8/8/8/Rb2K2R b KQkq - 0 1", {"e8c8"}, {"e8g8"}},
    {"r3k2r/8/5B2/8/8/8/8/Rb2K2R b KQkq - 0 1", {"e8g8"}, {"e8c8"}},
    {"r3k2r/8/5B2/8/8/8/8/Rb2K2R b KQ - 0 1", {}, {"e8c8", "e8g8"}},
};

const std::vector<CastleTestPosition> CHESS960_CASTLE_POSITIONS = {
    {"r3k2r/8/8/8/8/8/8/R3K2R w HAha - 0 1", {"e1h1", "e1a1"}, {}},
    {"r3k2r/8/8/8/8/8/8/R3K2R b HAha - 0 1", {"e8h8", "e8a8"}, {}},
    {"1r2k2r/8/8/8/8/8/8/2R1KR2 w FC - 0 1", {"e1f1", "e1c1"}, {}},
    {"1r2k2r/8/8/8/8/8/8/2R1KR2 b hb - 0 1", {}, {"e8h8", "e8b8"}},
    {"1r2k2r/8/8/8/8/8/4b3/2R1KR2 w FC - 0 1", {}, {"e1c1"}},
    {"1r2k2r/8/8/8/8/8/8/2RBKR2 w FC - 0 1", {}, {"e1c1"}},
    {"1r4kr/8/8/8/8/8/8/1R4KR w HBhb - 0 1", {"g1h1", "g1b1"}, {}},
    {"2r3kr/8/8/8/8/8/8/2R3KR w HChc - 0 1", {}, {"g1c1"}},
    {"r1k1r3/8/8/8/8/8/8/R1K1R3 w EAea - 0 1", {}, {"c1e1"}},
    {"1rk5/8/8/8/8/8/8/1RK5 w Bb - 0 1", {"c1b1"}, {}},
    {"2rk4/8/8/8/8/8/8/2RK4 w Cc - 0 1", {}, {"d1c1"}},
    {"1r1k4/8/8/8/8/8/8/2RK4 w C - 0 1", {"d1c1"}, {}},
    {"1r1knrbb/p1nppp1p/qpp3p1/8/5P2/3P1NP1/PPP1PB1P/NRQK1R1B w BFbf - 1 6", {"d1f1"}, {"d1b1"}},
    {"r1k1r3/p1qp1b1n/1p1n1bpp/3Ppp2/P1P5/2N2PP1/1PQN2BP/R2KR1B1 b EAea - 0 14", {"c8e8", "c8a8"}, {}},
    {"bbnnr1kr/p2p2pp/1p2p3/2p1P3/8/1PN5/P1PP2PP/BBN1RK1R b HEhe - 0 7", {"g8h8"}, {"g8e8"}},
};

class CastleTest : public ::testing::Test {
protected:
    void SetUp() override {
        TestUtils::initialize();
    }
};

TEST_F(CastleTest, StandardCastling) {
    for (const auto &testPos : STANDARD_CASTLE_POSITIONS) {
        Position pos(testPos.fen, false);
        std::set<std::string> engineMoves = TestUtils::getAllMovesAsStrings(pos);

        for (const std::string &move : testPos.findMoves) {
            EXPECT_TRUE(engineMoves.count(move) > 0)
                << "Move " << move
                << " should be present in position " << testPos.fen;
        }

        for (const std::string &move : testPos.avoidMoves) {
            EXPECT_TRUE(engineMoves.count(move) == 0)
                << "Move " << move
                << " should NOT be present in position " << testPos.fen;
        }
    }
}

TEST_F(CastleTest, Chess960Castling) {
    for (const auto &testPos : CHESS960_CASTLE_POSITIONS) {
        Position pos(testPos.fen, true);
        std::set<std::string> engineMoves = TestUtils::getAllMovesAsStrings(pos);

        for (const std::string &move : testPos.findMoves) {
            EXPECT_TRUE(engineMoves.count(move) > 0)
                << "Move " << move
                << " should be present in position " << testPos.fen;
        }

        for (const std::string &move : testPos.avoidMoves) {
            EXPECT_TRUE(engineMoves.count(move) == 0)
                << "Move " << move
                << " should NOT be present in position " << testPos.fen;
        }
    }
}
