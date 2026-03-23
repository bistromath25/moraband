#include "movegen.h"
#include "position.h"
#include "test_utils.h"
#include <gtest/gtest.h>

class PositionTest : public ::testing::Test {
protected:
    void SetUp() override {
        TestUtils::initialize();
    }
};

TEST_F(PositionTest, FenParsing) {
    Position pos("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");

    EXPECT_EQ(pos.getOurColor(), WHITE) << "Should be white's turn";
    EXPECT_EQ(pos.getPieceCount<PIECETYPE_PAWN>(WHITE), 8) << "White should have 8 pawns";
    EXPECT_EQ(pos.getPieceCount<PIECETYPE_PAWN>(BLACK), 8) << "Black should have 8 pawns";
}

TEST_F(PositionTest, MakeMove) {
    Position pos("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");

    U64 originalKey = pos.getKey();
    Move e4 = makeMove(E2, E4);

    pos.makeMove(e4);

    EXPECT_NE(pos.getKey(), originalKey) << "Position key should change after move";
    EXPECT_EQ(pos.getOurColor(), BLACK) << "Should be black's turn after move";

    EXPECT_EQ(pos.onSquare(E4), PIECETYPE_PAWN) << "Pawn should be on e4";
    EXPECT_EQ(pos.onSquare(E2), PIECETYPE_NONE) << "e2 should be empty";
}

TEST_F(PositionTest, IsLegal) {
    Position pos("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");

    Move e4 = makeMove(E2, E4);

    EXPECT_TRUE(pos.isLegal(e4)) << "e2e4 should be legal";
    EXPECT_TRUE(TestUtils::containsMove(pos, to_string(e4))) << "e2e4 should be in the legal move list";
}

TEST_F(PositionTest, InCheck) {
    Position pos("rnbqkbnr/ppp1pppp/3p4/1B6/4P3/8/PPPP1PPP/RNBQK1NR b KQkq - 1 2");
    EXPECT_TRUE(pos.inCheck()) << "Position should be in check";

    Position pos2("r1bqkbnr/pppppppp/4N3/8/8/3n4/PPPPPPPP/RNBQKB1R w KQkq - 6 4");
    EXPECT_TRUE(pos2.inCheck()) << "Position should be in check";
}

TEST_F(PositionTest, Attacked) {
    Position pos("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
    EXPECT_FALSE(pos.attacked(E4)) << "e4 should not be attacked";

    Position pos2("rnbqkb1r/pppppppp/5n2/8/4P3/8/PPPP1PPP/RNBQKBNR w KQkq - 0 2");
    EXPECT_TRUE(pos2.attacked(E4)) << "e4 should be attacked by knight";
}

TEST_F(PositionTest, CastlingRights) {
    Position pos("r3k2r/8/8/8/8/8/8/R3K2R w KQkq - 0 1");

    EXPECT_TRUE(pos.canCastleKingside()) << "Should be able to castle kingside";
    EXPECT_TRUE(pos.canCastleQueenside()) << "Should be able to castle queenside";
}

TEST_F(PositionTest, PieceCounts) {
    Position pos("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");

    EXPECT_EQ(pos.getPieceCount<PIECETYPE_QUEEN>(WHITE), 1) << "White should have 1 queen";
    EXPECT_EQ(pos.getPieceCount<PIECETYPE_QUEEN>(BLACK), 1) << "Black should have 1 queen";
    EXPECT_EQ(pos.getPieceCount<PIECETYPE_ROOK>(WHITE), 2) << "White should have 2 rooks";
    EXPECT_EQ(pos.getPieceCount<PIECETYPE_ROOK>(BLACK), 2) << "Black should have 2 rooks";
}

TEST_F(PositionTest, FenOutput) {
    std::string fen = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";
    Position pos(fen);

    std::string outputFen = pos.getFen();

    EXPECT_EQ(outputFen.substr(0, fen.find(' ')), fen.substr(0, fen.find(' ')))
        << "FEN output should match input";
}

TEST_F(PositionTest, KingSquare) {
    Position pos("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");

    EXPECT_EQ(pos.getKingSquare(WHITE), E1) << "White king should be on e1";
    EXPECT_EQ(pos.getKingSquare(BLACK), E8) << "Black king should be on e8";
}

TEST_F(PositionTest, OccupancyBitboards) {
    Position pos("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");

    U64 whiteOccupancy = pos.getOccupancyBB(WHITE);
    U64 blackOccupancy = pos.getOccupancyBB(BLACK);
    U64 allOccupancy = pos.getOccupancyBB();
    U64 empty = pos.getEmptyBB();

    EXPECT_GT(whiteOccupancy, 0) << "White should have pieces";
    EXPECT_GT(blackOccupancy, 0) << "Black should have pieces";
    EXPECT_EQ(allOccupancy, whiteOccupancy | blackOccupancy) << "All occupancy should be union";
    EXPECT_EQ(empty, ~allOccupancy) << "Empty should be complement of all occupancy";
}
