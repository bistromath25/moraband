/**
 * Moraband, known in antiquity as Korriban, was an 
 * Outer Rim planet that was home to the ancient Sith 
 **/

#include "nnue.h"
#include "defs.h"
#include "position.h"
#include <cmath>
#include <fstream>

#if defined(__APPLE__) && defined(__aarch64__)
#include <arm_neon.h>
#define USE_NEON
#endif

namespace NNUE {

    static float fc1_weight[INPUT_FEATURE_SIZE][HIDDEN_SIZE];
    static float fc1_bias[HIDDEN_SIZE];
    static float fc2_weight[HIDDEN_SIZE];
    static float fc2_bias;

    /** Feature index using fixed white perspective */
    int getPieceFeatureIndex(Color c, PieceType p, Square sq) {
        int base = (c == WHITE) ? 0 : 6;
        return flip_file(sq) * NUM_PIECE_TYPES + base + p;
    }

    void load_weights(const std::string &path) {
        std::ifstream in(path, std::ios::binary);
        if (!in) {
            throw std::runtime_error("Failed to open file: " + path);
        }

        const size_t size = INPUT_FEATURE_SIZE * HIDDEN_SIZE + HIDDEN_SIZE + HIDDEN_SIZE + 1;
        std::vector<float> buffer(size);
        in.read(reinterpret_cast<char *>(buffer.data()), size * sizeof(float));
        if (in.gcount() != static_cast<std::streamsize>(size * sizeof(float))) {
            throw std::runtime_error("Unexpected end of file: " + path);
        }

        size_t offset = 0;
        std::memcpy(fc1_weight, &buffer[offset], INPUT_FEATURE_SIZE * HIDDEN_SIZE * sizeof(float));
        offset += INPUT_FEATURE_SIZE * HIDDEN_SIZE;
        std::memcpy(fc1_bias, &buffer[offset], HIDDEN_SIZE * sizeof(float));
        offset += HIDDEN_SIZE;
        std::memcpy(fc2_weight, &buffer[offset], HIDDEN_SIZE * sizeof(float));
        offset += HIDDEN_SIZE;
        fc2_bias = buffer[offset];

        std::cout << "Loaded network from " << path << "\n"
                  << std::endl;
    }

    Accumulator::Accumulator() {
        std::memcpy(values.data(), fc1_bias, HIDDEN_SIZE * sizeof(float));
    }

    Accumulator::Accumulator(const Position &s) {
        std::memcpy(values.data(), fc1_bias, HIDDEN_SIZE * sizeof(float));
        U64 occWhite = s.getOccupancyBB(WHITE);
        U64 occBlack = s.getOccupancyBB(BLACK);

        while (occWhite) {
            Square sq = pop_lsb(occWhite);
            PieceType p = s.onSquare(sq);
            int feat = getPieceFeatureIndex(WHITE, p, sq);
            const float *w = fc1_weight[feat];
            for (int i = 0; i < HIDDEN_SIZE; ++i) {
                values[i] += w[i];
            }
        }
        while (occBlack) {
            Square sq = pop_lsb(occBlack);
            PieceType p = s.onSquare(sq);
            int feat = getPieceFeatureIndex(BLACK, p, sq);
            const float *w = fc1_weight[feat];
            for (int i = 0; i < HIDDEN_SIZE; ++i) {
                values[i] += w[i];
            }
        }
    }

    NNUE::NNUE() {}

    NNUE::NNUE(const Position &s) : acc(s) {}

    NNUE::NNUE(const NNUE &n) {
        acc = n.acc;
    }

    void add_feature_simd(float *values, const float *w) {
#ifdef USE_NEON
        for (int i = 0; i < HIDDEN_SIZE; i += 4) {
            float32x4_t values_vec = vld1q_f32(&values[i]);
            float32x4_t w_vec = vld1q_f32(&w[i]);
            values_vec = vaddq_f32(values_vec, w_vec);
            vst1q_f32(&values[i], values_vec);
        }
#else
        for (int i = 0; i < HIDDEN_SIZE; ++i) {
            values[i] += w[i];
        }
#endif
    }

    void remove_feature_simd(float *values, const float *w) {
#ifdef USE_NEON
        for (int i = 0; i < HIDDEN_SIZE; i += 4) {
            float32x4_t values_vec = vld1q_f32(&values[i]);
            float32x4_t w_vec = vld1q_f32(&w[i]);
            values_vec = vsubq_f32(values_vec, w_vec);
            vst1q_f32(&values[i], values_vec);
        }
#else
        for (int i = 0; i < HIDDEN_SIZE; ++i) {
            values[i] -= w[i];
        }
#endif
    }

    void add_feature(std::array<float, HIDDEN_SIZE> &values, int idx) {
        const float *w = fc1_weight[idx];
        add_feature_simd(values.data(), w);
    }

    void remove_feature(std::array<float, HIDDEN_SIZE> &values, int idx) {
        const float *w = fc1_weight[idx];
        remove_feature_simd(values.data(), w);
    }

    void NNUE::addPiece(Color c, PieceType p, Square sq) {
        int feat = getPieceFeatureIndex(c, p, sq);
        add_feature(acc.values, feat);
    }

    void NNUE::removePiece(Color c, PieceType p, Square sq) {
        int feat = getPieceFeatureIndex(c, p, sq);
        remove_feature(acc.values, feat);
    }

    void NNUE::movePiece(Color c, PieceType p, Square src, Square dst) {
        int featSrc = getPieceFeatureIndex(c, p, src);
        int featDst = getPieceFeatureIndex(c, p, dst);
        remove_feature(acc.values, featSrc);
        add_feature(acc.values, featDst);
    }

    int NNUE::evaluate(Color c) const {
        const float *w_stm = fc1_weight[INPUT_FEATURE_SIZE - 1];
        const float sign = (c == WHITE) ? 1.0f : -1.0f;
#ifdef USE_NEON
        float32x4_t sum_vec = vdupq_n_f32(0.0f);
        float32x4_t zero = vdupq_n_f32(0.0f);
        float32x4_t one = vdupq_n_f32(1.0f);
        float32x4_t sign_vec = vdupq_n_f32(sign);

        for (int i = 0; i < HIDDEN_SIZE; i += 4) {
            float32x4_t x = vld1q_f32(&acc.values[i]);
            float32x4_t w = vld1q_f32(&fc2_weight[i]);
            float32x4_t wstm = vld1q_f32(&w_stm[i]);
            x = vmlaq_f32(x, wstm, sign_vec);
            x = vmaxq_f32(x, zero);
            x = vminq_f32(x, one);
            float32x4_t product = vmulq_f32(x, w);
            sum_vec = vaddq_f32(sum_vec, product);
        }

        float32x2_t lo = vget_low_f32(sum_vec);
        float32x2_t hi = vget_high_f32(sum_vec);
        float32x2_t sum2 = vpadd_f32(lo, hi);
        float sum = vget_lane_f32(sum2, 0) + vget_lane_f32(sum2, 1);
#else
        float sum = 0.0f;
        for (int i = 0; i < HIDDEN_SIZE; ++i) {
            float activated = std::min(1.0f, std::max(0.0f, acc.values[i] + sign * w_stm[i]));
            sum += fc2_weight[i] * activated;
        }
#endif
        sum += fc2_bias;
        return static_cast<int>(std::round((c == WHITE ? sum : -sum) * SCALE));
    }

} // namespace NNUE
