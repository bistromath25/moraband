/**
 * Moraband, known in antiquity as Korriban, was an 
 * Outer Rim planet that was home to the ancient Sith 
 **/

#include "nnue.h"
#include "defs.h"
#include "position.h"
#include <cmath>
#include <cstring>
#include <fstream>
#include <vector>

namespace NNUE {

    constexpr int QA = 255;
    constexpr int QB = 64;

    static int16_t fc1_weight[INPUT_FEATURE_SIZE][HIDDEN_SIZE];
    static int16_t fc1_bias[HIDDEN_SIZE];
    static int16_t fc2_weight[HIDDEN_SIZE * 2];
    static int16_t fc2_bias;

    /** Feature index using fixed white perspective */
    int getPieceFeatureIndex(Color c, PieceType p, Square sq) {
        return c * 64 * 6 + p * 64 + sq;
    }

    void load_weights(const std::string &path) {
        std::ifstream in(path, std::ios::binary);
        if (!in) {
            throw std::runtime_error("Failed to open file: " + path);
        }

        in.read(reinterpret_cast<char *>(fc1_weight), sizeof(fc1_weight));
        in.read(reinterpret_cast<char *>(fc1_bias), sizeof(fc1_bias));
        in.read(reinterpret_cast<char *>(fc2_weight), sizeof(fc2_weight));
        in.read(reinterpret_cast<char *>(&fc2_bias), sizeof(fc2_bias));

        if (!in) {
            throw std::runtime_error("Unexpected end of file: " + path);
        }

        std::cout << "Loaded network from " << path << "\n"
                  << std::endl;
    }

    Accumulator::Accumulator() {
        std::memcpy(white.data(), fc1_bias, HIDDEN_SIZE * sizeof(int16_t));
        std::memcpy(black.data(), fc1_bias, HIDDEN_SIZE * sizeof(int16_t));
    }

    Accumulator::Accumulator(const Position &s) {
        std::memcpy(white.data(), fc1_bias, HIDDEN_SIZE * sizeof(int16_t));
        std::memcpy(black.data(), fc1_bias, HIDDEN_SIZE * sizeof(int16_t));
        U64 occWhite = s.getOccupancyBB(WHITE);
        U64 occBlack = s.getOccupancyBB(BLACK);

        while (occWhite) {
            Square sq = pop_lsb(occWhite);
            PieceType p = s.onSquare(sq);
            int w_feat = getPieceFeatureIndex(WHITE, p, sq);
            int b_feat = getPieceFeatureIndex(BLACK, p, flip(sq));
            for (int i = 0; i < HIDDEN_SIZE; ++i) {
                white[i] += fc1_weight[w_feat][i];
                black[i] += fc1_weight[b_feat][i];
            }
        }
        while (occBlack) {
            Square sq = pop_lsb(occBlack);
            PieceType p = s.onSquare(sq);
            int w_feat = getPieceFeatureIndex(BLACK, p, sq);
            int b_feat = getPieceFeatureIndex(WHITE, p, flip(sq));
            for (int i = 0; i < HIDDEN_SIZE; ++i) {
                white[i] += fc1_weight[w_feat][i];
                black[i] += fc1_weight[b_feat][i];
            }
        }
    }

    NNUE::NNUE() {}

    NNUE::NNUE(const Position &s) : acc(s) {}

    void NNUE::addPiece(Color c, PieceType p, Square sq) {
        int w_feat = getPieceFeatureIndex(c, p, sq);
        int b_feat = getPieceFeatureIndex(!c, p, flip(sq));
        for (int i = 0; i < HIDDEN_SIZE; ++i) {
            acc.white[i] += fc1_weight[w_feat][i];
            acc.black[i] += fc1_weight[b_feat][i];
        }
    }

    void NNUE::removePiece(Color c, PieceType p, Square sq) {
        int w_feat = getPieceFeatureIndex(c, p, sq);
        int b_feat = getPieceFeatureIndex(!c, p, flip(sq));
        for (int i = 0; i < HIDDEN_SIZE; ++i) {
            acc.white[i] -= fc1_weight[w_feat][i];
            acc.black[i] -= fc1_weight[b_feat][i];
        }
    }

    void NNUE::movePiece(Color c, PieceType p, Square src, Square dst) {
        removePiece(c, p, src);
        addPiece(c, p, dst);
    }

    int NNUE::evaluate(Color stm) const {
        const std::array<int16_t, HIDDEN_SIZE> &us = (stm == WHITE) ? acc.white : acc.black;
        const std::array<int16_t, HIDDEN_SIZE> &them = (stm == WHITE) ? acc.black : acc.white;
        int32_t sum = 0;

        for (int i = 0; i < HIDDEN_SIZE; ++i) {
            int16_t activated = std::max<int16_t>(0, std::min<int16_t>(QA, us[i]));
            int32_t screlu = (activated * activated) / QA;
            sum += fc2_weight[i] * screlu;
        }

        for (int i = 0; i < HIDDEN_SIZE; ++i) {
            int16_t activated = std::max<int16_t>(0, std::min<int16_t>(QA, them[i]));
            int32_t screlu = (activated * activated) / QA;
            sum += fc2_weight[HIDDEN_SIZE + i] * screlu;
        }

        sum += fc2_bias;
        return (sum * SCALE) / (QA * QB);
    }

} // namespace NNUE
