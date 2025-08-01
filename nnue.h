/**
 * Moraband, known in antiquity as Korriban, was an 
 * Outer Rim planet that was home to the ancient Sith 
 **/

#ifndef NNUE_H
#define NNUE_H

#include "defs.h"
#include <array>
#include <string>

class Position;

namespace NNUE {

    const std::string NNUE_PATH = "nnue.bin";
    constexpr int FEATURE_SIZE = 768;
    constexpr int HIDDEN_SIZE = 128;
    constexpr int SCALE = 1000;

    constexpr int NUM_PIECE_TYPES = 12;

    struct Accumulator {
        Accumulator();
        Accumulator(const Position &s, Color c);
        std::array<float, HIDDEN_SIZE> values;
    };

    class NNUE {
    public:
        NNUE();
        NNUE(const Position &s);
        NNUE(const NNUE &n);
        void add_feature(Color pov, int idx);
        void remove_feature(Color pov, int idx);
        void addPiece(Color c, PieceType p, Square sq);
        void removePiece(Color c, PieceType p, Square sq);
        void movePiece(Color c, PieceType p, Square src, Square dst);
        int evaluate(Color pov);

    private:
        Accumulator white, black;
    };

    void load_weights(const std::string &path = NNUE_PATH);

} // namespace NNUE

#endif
