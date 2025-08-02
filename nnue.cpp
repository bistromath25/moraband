/**
 * Moraband, known in antiquity as Korriban, was an 
 * Outer Rim planet that was home to the ancient Sith 
 **/

#include "nnue.h"
#include "defs.h"
#include "position.h"
#include <cmath>
#include <fstream>

namespace NNUE {

    static float fc1_weight[FEATURE_SIZE][HIDDEN_SIZE];
    static float fc1_bias[HIDDEN_SIZE];
    static float fc2_weight[HIDDEN_SIZE];
    static float fc2_bias;

    void load_weights(const std::string &path) {
        std::ifstream in(path, std::ios::binary);
        if (!in) {
            throw std::runtime_error("Failed to open file: " + path);
        }

        const size_t size = FEATURE_SIZE * HIDDEN_SIZE + HIDDEN_SIZE + HIDDEN_SIZE + 1;
        std::vector<float> buffer(size);
        in.read(reinterpret_cast<char*>(buffer.data()), size * sizeof(float));
        if (in.gcount() != static_cast<std::streamsize>(size * sizeof(float))) {
            throw std::runtime_error("Unexpected end of file: " + path);
        }

        size_t offset = 0;
        std::memcpy(fc1_weight, &buffer[offset], FEATURE_SIZE * HIDDEN_SIZE * sizeof(float));
        offset += FEATURE_SIZE * HIDDEN_SIZE;
        std::memcpy(fc1_bias, &buffer[offset], HIDDEN_SIZE * sizeof(float));
        offset += HIDDEN_SIZE;
        std::memcpy(fc2_weight, &buffer[offset], HIDDEN_SIZE * sizeof(float));
        offset += HIDDEN_SIZE;
        fc2_bias = buffer[offset];

        std::cout << "Loaded network from " << NNUE_PATH << "\n"
                  << std::endl;
    }

    int getPieceFeatureIndex(Color c, PieceType p, Square sq, Color pov) {
        int base = (c == WHITE) ? 0 : 6;
        return (pov == WHITE ? sq : flip(sq)) * NUM_PIECE_TYPES + base + p;
    }

    Accumulator::Accumulator() {
        values.fill(0);
    }

    Accumulator::Accumulator(const Position &s, Color c) {
        values.fill(0);
        Color pov = c;
        U64 occWhite = s.getOccupancyBB(WHITE);
        U64 occBlack = s.getOccupancyBB(BLACK);

        while (occWhite) {
            Square sq = pop_lsb(occWhite);
            PieceType p = s.onSquare(sq);
            int feat = getPieceFeatureIndex(WHITE, p, sq, pov);
            const float *w = fc1_weight[feat];
            for (int i = 0; i < HIDDEN_SIZE; ++i) {
                values[i] += w[i];
            }
        }
        while (occBlack) {
            Square sq = pop_lsb(occBlack);
            PieceType p = s.onSquare(sq);
            int feat = getPieceFeatureIndex(BLACK, p, sq, pov);
            const float *w = fc1_weight[feat];
            for (int i = 0; i < HIDDEN_SIZE; ++i) {
                values[i] += w[i];
            }
        }
    }

    NNUE::NNUE() {}

    NNUE::NNUE(const Position &s) : white(s, WHITE), black(s, BLACK) {}

    NNUE::NNUE(const NNUE &n) {
        white = n.white;
        black = n.black;
    }

    void NNUE::add_feature(Color pov, int idx) {
        Accumulator &acc = (pov == WHITE) ? white : black;
        const float *w = fc1_weight[idx];
        for (int i = 0; i < HIDDEN_SIZE; ++i) {
            acc.values[i] += w[i];
        }
    }

    void NNUE::remove_feature(Color pov, int idx) {
        Accumulator &acc = (pov == WHITE) ? white : black;
        const float *w = fc1_weight[idx];
        for (int i = 0; i < HIDDEN_SIZE; ++i) {
            acc.values[i] -= w[i];
        }
    }

    void NNUE::addPiece(Color c, PieceType p, Square sq) {
        for (Color pov : {WHITE, BLACK}) {
            int feat = getPieceFeatureIndex(c, p, sq, pov);
            add_feature(pov, feat);
        }
    }

    void NNUE::removePiece(Color c, PieceType p, Square sq) {
        for (Color pov : {WHITE, BLACK}) {
            int feat = getPieceFeatureIndex(c, p, sq, pov);
            remove_feature(pov, feat);
        }
    }

    void NNUE::movePiece(Color c, PieceType p, Square src, Square dst) {
        for (Color pov : {WHITE, BLACK}) {
            int featSrc = getPieceFeatureIndex(c, p, src, pov);
            int featDst = getPieceFeatureIndex(c, p, dst, pov);
            remove_feature(pov, featSrc);
            add_feature(pov, featDst);
        }
    }

    int NNUE::evaluate(Color pov) {
        Accumulator &acc = (pov == WHITE) ? white : black;
        float sum = fc2_bias;
        for (int i = 0; i < HIDDEN_SIZE; ++i) {
            float activated = std::min(1.0f, std::max(0.0f, acc.values[i]));
            sum += fc2_weight[i] * activated;
        }
        return static_cast<int>(std::round(sum * SCALE));
    }

}; // namespace NNUE
