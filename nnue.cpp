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

    std::vector<float> load_weights_file(const std::string &path) {
        std::ifstream in(path, std::ios::binary);
        if (!in) {
            throw std::runtime_error("Failed to open file: " + path);
        }

        in.seekg(0, std::ios::end);
        size_t size = in.tellg() / sizeof(float);
        in.seekg(0);

        std::vector<float> weights(size);
        in.read(reinterpret_cast<char *>(weights.data()), size * sizeof(float));
        return weights;
    }

    void load_weights(const std::string &path) {
        std::vector<float> weights = load_weights_file(path);
        size_t offset = 0;

        // fc1_weight
        for (int i = 0; i < FEATURE_SIZE; ++i) {
            for (int j = 0; j < HIDDEN_SIZE; ++j) {
                fc1_weight[i][j] = weights[offset++];
            }
        }
        // fc1_bias
        for (int j = 0; j < HIDDEN_SIZE; ++j) {
            fc1_bias[j] = weights[offset++];
        }
        // fc2_weight
        for (int j = 0; j < HIDDEN_SIZE; ++j) {
            fc2_weight[j] = weights[offset++];
        }
        // fc2_bias
        fc2_bias = weights[offset++];

        std::cout << "Loaded network " << NNUE_PATH << "\n"
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
