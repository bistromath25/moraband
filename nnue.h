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
    constexpr int INPUT_FEATURE_SIZE = 768;
    constexpr int HIDDEN_SIZE = 256;
    constexpr int SCALE = 400;

    struct Accumulator {
        Accumulator();
        Accumulator(const Position &s);
        std::array<int16_t, HIDDEN_SIZE> white, black;
    };

    class NNUE {
    public:
        NNUE();
        NNUE(const Position &s);
        void addPiece(Color c, PieceType p, Square sq);
        void removePiece(Color c, PieceType p, Square sq);
        void movePiece(Color c, PieceType p, Square src, Square dst);
        int evaluate(Color c) const;

    private:
        Accumulator acc;
    };

    void load_weights(const std::string &path = NNUE_PATH);

} // namespace NNUE

#endif
