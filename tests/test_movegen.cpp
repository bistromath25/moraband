#include "movegen.h"
#include "position.h"
#include "test_utils.h"
#include <gtest/gtest.h>

class MoveGenTest : public ::testing::Test {
protected:
    void SetUp() override {
        TestUtils::initialize();
    }
};

TEST_F(MoveGenTest, StartingPosition) {
    Position pos("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");

    MoveList moveList(pos);
    int moveCount = moveList.size();

    EXPECT_EQ(moveCount, 20) << "Starting position should have exactly 20 moves";
}

TEST_F(MoveGenTest, SpecificPosition) {
    Position pos("r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1");

    MoveList moveList(pos);
    int moveCount = moveList.size();

    EXPECT_EQ(moveCount, 48) << "Position should have exactly 48 moves";
}

TEST_F(MoveGenTest, AllMovesAreLegal) {
    Position pos("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");

    MoveList moveList(pos);
    std::vector<Move> moves = TestUtils::getAllMoves(pos);

    for (const Move &move : moves) {
        Position testPos = pos;
        EXPECT_TRUE(testPos.isLegal(move))
            << "Move " << to_string(move) << " should be legal";
    }
}

TEST_F(MoveGenTest, MoveGenerationInCheck) {
    Position pos("rnb1kbnr/pppp1ppp/8/4p3/4PP1q/8/PPPP2PP/RNBQKBNR w KQkq - 0 1");

    EXPECT_TRUE(pos.inCheck()) << "Position should be in check";

    MoveList moveList(pos);
    std::vector<Move> moves = TestUtils::getAllMoves(pos);

    for (const Move &move : moves) {
        EXPECT_TRUE(pos.isLegal(move))
            << "All generated moves should be legal when in check";
    }
}

TEST_F(MoveGenTest, MoveGenerationInDoubleCheck) {
    Position pos("k3r3/8/8/8/8/2b5/8/4K3 w - - 0 1");

    ASSERT_TRUE(pos.inDoubleCheck()) << "Position must be in double check for this test";

    MoveList moveList(pos);
    std::vector<Move> moves = TestUtils::getAllMoves(pos);

    for (const Move &move : moves) {
        EXPECT_EQ(pos.onSquare(getSrc(move)), PIECETYPE_KING)
            << "In double check, only king moves should be legal";
    }
}

TEST_F(MoveGenTest, PawnMoves) {
    Position pos("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");

    std::set<std::string> moves = TestUtils::getAllMovesAsStrings(pos);

    EXPECT_TRUE(TestUtils::containsMove(pos, "e2e4")) << "Should have e2e4";
    EXPECT_TRUE(TestUtils::containsMove(pos, "e2e3")) << "Should have e2e3";
    EXPECT_TRUE(TestUtils::containsMove(pos, "a2a3")) << "Should have a2a3";
    EXPECT_TRUE(TestUtils::containsMove(pos, "a2a4")) << "Should have a2a4";
}

TEST_F(MoveGenTest, CaptureMoves) {
    Position pos("rnbqkb1r/pppp1ppp/5n2/4p3/4P3/5N2/PPPP1PPP/RNBQKB1R w KQkq e6 0 3");

    int captureCount = TestUtils::countCaptureMoves(pos);

    EXPECT_EQ(captureCount, 1) << "Should have exactly 1 capture move";
}

TEST_F(MoveGenTest, QuietMoves) {
    Position pos("rnbqkb1r/pppp1ppp/5n2/4p3/4P3/5N2/PPPP1PPP/RNBQKB1R w KQkq e6 0 3");

    int quietCount = TestUtils::countQuietMoves(pos);

    EXPECT_EQ(quietCount, 26) << "Should have exactly 26 quiet moves";
}

TEST_F(MoveGenTest, CastlingMoves) {
    Position pos("r3k2r/8/8/8/8/8/8/R3K2R w KQkq - 0 1");

    std::set<std::string> moves = TestUtils::getAllMovesAsStrings(pos);

    bool hasKingsideCastle = moves.count("e1g1") > 0;
    bool hasQueensideCastle = moves.count("e1c1") > 0;

    EXPECT_TRUE(hasKingsideCastle && hasQueensideCastle)
        << "Should have exactly 2 castling moves available";
}

TEST_F(MoveGenTest, PromotionMoves) {
    Position pos("k7/4P3/8/8/8/8/8/4K3 w - - 0 1");

    std::vector<Move> moves = TestUtils::getAllMoves(pos);

    bool hasPromotion = false;
    for (const Move &move : moves) {
        if (isPromotion(move)) {
            hasPromotion = true;
            break;
        }
    }

    EXPECT_TRUE(hasPromotion) << "Should have moves (promotion check)";
}

TEST_F(MoveGenTest, EnPassantMoves) {
    Position pos("rnbqkbnr/ppp1pppp/8/3pP3/8/8/PPPP1PPP/RNBQKBNR w KQkq d6 0 3");

    bool hasEnPassant = false;
    std::vector<Move> moves = TestUtils::getAllMoves(pos);
    for (const Move &move : moves) {
        if (pos.isEnPassant(move)) {
            hasEnPassant = true;
            break;
        }
    }

    EXPECT_TRUE(hasEnPassant) << "Should have en passant move";
}

TEST_F(MoveGenTest, MoveGenerationAfterMakeMove) {
    Position pos("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");

    Move e4 = makeMove(E2, E4);
    pos.makeMove(e4);

    MoveList moveList(pos);
    int moveCount = moveList.size();

    EXPECT_EQ(moveCount, 20) << "After e4, should have 20 moves for black";
}

TEST_F(MoveGenTest, MoveListSizeConsistency) {
    Position pos("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");

    MoveList moveList(pos);
    size_t reportedSize = moveList.size();

    size_t actualSize = 0;
    while (moveList.size() > 0) {
        moveList.pop();
        ++actualSize;
    }

    EXPECT_EQ(reportedSize, actualSize)
        << "Reported size should match actual move count";
}

TEST_F(MoveGenTest, MinimalPosition) {
    Position pos("4k3/8/8/8/8/8/8/4K3 w - - 0 1");

    MoveList moveList(pos);
    int moveCount = moveList.size();

    EXPECT_EQ(moveCount, 5) << "King on e1 should have exactly 5 moves";
}

TEST_F(MoveGenTest, KingOnlyPosition) {
    Position pos("4k3/8/8/8/8/8/8/7K w - - 0 1");

    MoveList moveList(pos);
    int moveCount = moveList.size();

    EXPECT_EQ(moveCount, 3) << "King in corner should have exactly 3 moves";
}

TEST_F(MoveGenTest, NoDuplicateMoves) {
    Position pos("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");

    std::set<std::string> moves = TestUtils::getAllMovesAsStrings(pos);
    MoveList moveList(pos);
    size_t moveListSize = moveList.size();

    EXPECT_EQ(moves.size(), moveListSize)
        << "Should not have duplicate moves";
}
