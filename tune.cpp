/**
 * Moraband, known in antiquity as Korriban, was an 
 * Outer Rim planet that was home to the ancient Sith 
 **/

#include "tune.h"
#include "defs.h"
#include "eval.h"
#include "search.h"
#include <chrono>
#include <cmath>
#include <fstream>
#include <iostream>
#include <random>
#include <thread>
#include <vector>

std::string current_time() {
    auto now = std::chrono::system_clock::now();
    std::time_t t_now = std::chrono::system_clock::to_time_t(now);
    std::tm tm_now = *std::localtime(&t_now);
    std::ostringstream oss;
    oss << std::put_time(&tm_now, "%H:%M:%S");
    return oss.str();
}

std::string format_time(double seconds) {
    int mins = static_cast<int>(seconds) / 60;
    int secs = static_cast<int>(seconds) % 60;
    std::ostringstream oss;
    oss << std::setw(2) << std::setfill('0') << mins
        << ":" << std::setw(2) << std::setfill('0') << secs;
    return oss.str();
}

struct Parameter {
    std::string name;
    int *ptr;
    double value;
};

struct Input {
    Position s;
    long double result;
};

static std::vector<Input> data;

void get_fen_info(std::string &s, std::vector<std::string> &v) {
    auto i = s.find_last_of(' ');
    if (i != std::string::npos) {
        v.push_back(s.substr(0, i));
        v.push_back(s.substr(i + 1));
    }
}

inline long double sigmoid(long double x, long double k) {
    long double scale = (k * M_LN10) / 400.0L;
    return 1.0L / (1.0L + expl(-scale * x));
}

long double evaluate_error(const std::vector<Parameter> &P, long double k) {
    for (auto &p : P) {
        *p.ptr = p.value;
    }

    std::vector<long double> thread_error(NUM_THREADS, 0.0L);
    std::vector<uint64_t> thread_count(NUM_THREADS, 0);

    auto worker = [&](int tid) {
        long double local_error = 0.0L;
        uint64_t local_count = 0;

        for (size_t i = tid; i < data.size(); i += NUM_THREADS) {
            auto &e = data[i];
            if (e.s.inCheck()) continue;

            SearchInfo si;
            si.infinite = true;
            global_info[tid].clear();

            int q = qsearch(e.s, si, global_info[tid], 0, NEG_INF, POS_INF);
            if (e.s.getOurColor() == Color::BLACK)
                q = -q;

            long double p = sigmoid(q, k);
            long double diff = p - e.result;
            local_error += diff * diff;
            ++local_count;
        }

        thread_error[tid] = local_error;
        thread_count[tid] = local_count;
    };

    std::vector<std::thread> threads;
    for (int t = 0; t < NUM_THREADS; ++t) {
        threads.emplace_back(worker, t);
    }
    for (auto &t : threads) {
        t.join();
    }

    long double total_error = 0.0L;
    uint64_t total_count = 0;
    for (int t = 0; t < NUM_THREADS; ++t) {
        total_error += thread_error[t];
        total_count += thread_count[t];
    }

    return total_count ? total_error / total_count : 0.0L;
}

void spsa_tune(std::vector<Parameter> &P, long double k) {
    const int N = P.size();
    std::mt19937_64 rng(123456);

    const double a0 = 100.0;
    const double c0 = 10.0;
    const double alpha = 0.602;
    const double gamma = 0.101;
    const int num_iterations = 10000;
    const int print_every = num_iterations / 20;

    auto start_time = std::chrono::high_resolution_clock::now();

    for (int i = 1; i <= num_iterations; ++i) {
        double a = a0 / std::pow(i, alpha);
        double c = c0 / std::pow(i, gamma);

        std::vector<int> delta(N);
        for (int i = 0; i < N; ++i) {
            delta[i] = (rng() & 1) ? 1 : -1;
        }

        std::vector<Parameter> Pplus = P;
        std::vector<Parameter> Pminus = P;

        for (int i = 0; i < N; ++i) {
            Pplus[i].value = P[i].value + c * delta[i];
            Pminus[i].value = P[i].value - c * delta[i];
        }

        long double errorPlus = evaluate_error(Pplus, k);
        long double errorMinus = evaluate_error(Pminus, k);

        std::vector<double> g(N);
        for (int i = 0; i < N; ++i) {
            g[i] = (double) (errorPlus - errorMinus) / (2.0 * c * delta[i]);
        }

        for (int i = 0; i < N; ++i) {
            P[i].value -= a * g[i];
        }

        if (i % print_every == 0 || i == num_iterations) {
            long double err = evaluate_error(P, k);
            auto now = std::chrono::high_resolution_clock::now();
            double elapsed = std::chrono::duration<double>(now - start_time).count();
            double eta_sec = (elapsed / i) * (num_iterations - i);
            std::cerr << "[" << current_time() << "] "
                      << "[Iteration " << i << "/" << num_iterations << "] "
                      << "Error = " << err
                      << " - ETA: " << format_time(eta_sec)
                      << "\n";
        }
    }
}

void set_parameters(std::vector<Parameter> &P) {
    auto add = [&](int *ptr, const std::string &name) {
        P.push_back({name, ptr, (double) *ptr});
    };

    // Material
    // add(&PAWN_WEIGHT.mg, "PAWN_WEIGHT.mg");
    add(&PAWN_WEIGHT.eg, "PAWN_WEIGHT.eg");
    add(&KNIGHT_WEIGHT.mg, "KNIGHT_WEIGHT.mg");
    add(&KNIGHT_WEIGHT.eg, "KNIGHT_WEIGHT.eg");
    add(&BISHOP_WEIGHT.mg, "BISHOP_WEIGHT.mg");
    add(&BISHOP_WEIGHT.eg, "BISHOP_WEIGHT.eg");
    add(&ROOK_WEIGHT.mg, "ROOK_WEIGHT.mg");
    add(&ROOK_WEIGHT.eg, "ROOK_WEIGHT.eg");
    add(&QUEEN_WEIGHT.mg, "QUEEN_WEIGHT.mg");
    add(&QUEEN_WEIGHT.eg, "QUEEN_WEIGHT.eg");
    add(&BISHOP_PAIR.mg, "BISHOP_PAIR.mg");
    add(&BISHOP_PAIR.eg, "BISHOP_PAIR.eg");
    add(&BAD_BISHOP.mg, "BAD_BISHOP.mg");
    add(&BAD_BISHOP.eg, "BAD_BISHOP.eg");
    add(&ROOK_OPEN_FILE.mg, "ROOK_OPEN_FILE.mg");
    add(&ROOK_OPEN_FILE.eg, "ROOK_OPEN_FILE.eg");
    add(&ROOK_ON_SEVENTH_RANK.mg, "ROOK_ON_SEVENTH_RANK.mg");
    add(&ROOK_ON_SEVENTH_RANK.eg, "ROOK_ON_SEVENTH_RANK.eg");
    add(&KNIGHT_OUTPOST.mg, "KNIGHT_OUTPOST.mg");
    add(&KNIGHT_OUTPOST.eg, "KNIGHT_OUTPOST.eg");
    add(&BISHOP_OUTPOST.mg, "BISHOP_OUTPOST.mg");
    add(&BISHOP_OUTPOST.eg, "BISHOP_OUTPOST.eg");
    for (int i = 0; i < 4; ++i) {
        for (int j = 0; j < 7; ++j) {
            add(&PAWN_PASSED[i][j].mg, "PAWN_PASSED[" + std::to_string(i) + "][" + std::to_string(j) + "].mg");
            add(&PAWN_PASSED[i][j].eg, "PAWN_PASSED[" + std::to_string(i) + "][" + std::to_string(j) + "].eg");
        }
    }
    add(&PAWN_PASSED_CANDIDATE.mg, "PAWN_PASSED_CANDIDATE.mg");
    add(&PAWN_PASSED_CANDIDATE.eg, "PAWN_PASSED_CANDIDATE.eg");
    add(&PAWN_CONNECTED.mg, "PAWN_CONNECTED.mg");
    add(&PAWN_CONNECTED.eg, "PAWN_CONNECTED.eg");
    add(&PAWN_ISOLATED.mg, "PAWN_ISOLATED.mg");
    add(&PAWN_ISOLATED.eg, "PAWN_ISOLATED.eg");
    add(&PAWN_DOUBLED.mg, "PAWN_DOUBLED.mg");
    add(&PAWN_DOUBLED.eg, "PAWN_DOUBLED.eg");
    add(&PAWN_FULL_BACKWARDS.mg, "PAWN_FULL_BACKWARDS.mg");
    add(&PAWN_FULL_BACKWARDS.eg, "PAWN_FULL_BACKWARDS.eg");
    add(&PAWN_BACKWARDS.mg, "PAWN_BACKWARDS.mg");
    add(&PAWN_BACKWARDS.eg, "PAWN_BACKWARDS.eg");
    add(&PAWN_SHIELD_CLOSE.mg, "PAWN_SHIELD_CLOSE.mg");
    add(&PAWN_SHIELD_CLOSE.eg, "PAWN_SHIELD_CLOSE.eg");
    add(&PAWN_SHIELD_FAR.mg, "PAWN_SHIELD_FAR.mg");
    add(&PAWN_SHIELD_FAR.eg, "PAWN_SHIELD_FAR.eg");
    add(&PAWN_SHIELD_MISSING.mg, "PAWN_SHIELD_MISSING.mg");
    add(&PAWN_SHIELD_MISSING.eg, "PAWN_SHIELD_MISSING.eg");
    add(&STRONG_PAWN_ATTACK.mg, "STRONG_PAWN_ATTACK.mg");
    add(&STRONG_PAWN_ATTACK.eg, "STRONG_PAWN_ATTACK.eg");
    add(&WEAK_PAWN_ATTACK.mg, "WEAK_PAWN_ATTACK.mg");
    add(&WEAK_PAWN_ATTACK.eg, "WEAK_PAWN_ATTACK.eg");
    add(&HANGING.mg, "HANGING.mg");
    add(&HANGING.eg, "HANGING.eg");
    add(&KNIGHT_PAWN_PENALTY.mg, "KNIGHT_PAWN_PENALTY.mg");
    add(&KNIGHT_PAWN_PENALTY.eg, "KNIGHT_PAWN_PENALTY.eg");
    add(&ROOK_PAWN_BONUS.mg, "ROOK_PAWN_BONUS.mg");
    add(&ROOK_PAWN_BONUS.eg, "ROOK_PAWN_BONUS.eg");

    // Mobility
    for (int i = 0; i < 9; ++i) {
        add(&KNIGHT_MOBILITY[i].mg, "KNIGHT_MOBILITY[" + std::to_string(i) + "].mg");
        add(&KNIGHT_MOBILITY[i].eg, "KNIGHT_MOBILITY[" + std::to_string(i) + "].eg");
    }
    for (int i = 0; i < 14; ++i) {
        add(&BISHOP_MOBILITY[i].mg, "BISHOP_MOBILITY[" + std::to_string(i) + "].mg");
        add(&BISHOP_MOBILITY[i].eg, "BISHOP_MOBILITY[" + std::to_string(i) + "].eg");
    }
    for (int i = 0; i < 15; ++i) {
        add(&ROOK_MOBILITY[i].mg, "ROOK_MOBILITY[" + std::to_string(i) + "].mg");
        add(&ROOK_MOBILITY[i].eg, "ROOK_MOBILITY[" + std::to_string(i) + "].eg");
    }
    for (int i = 0; i < 28; ++i) {
        add(&QUEEN_MOBILITY[i].mg, "QUEEN_MOBILITY[" + std::to_string(i) + "].mg");
        add(&QUEEN_MOBILITY[i].eg, "QUEEN_MOBILITY[" + std::to_string(i) + "].eg");
    }
}

void tune(const std::string &fensFile, int num_threads) {
    std::ifstream f(fensFile);
    std::string line;
    while (std::getline(f, line)) {
        std::vector<std::string> info;
        get_fen_info(line, info);

        Input e;
        e.s = Position(info[0]);
        if (info[1] == "1.0") {
            e.result = 1.0L;
        }
        else if (info[1] == "0.0") {
            e.result = 0.0L;
        }
        else {
            e.result = 0.5L;
        }

        data.push_back(e);
    }
    std::cerr << "Loaded " << data.size() << " positions\n";

    NUM_THREADS = num_threads;
    std::cerr << "Tuning with " << NUM_THREADS << " threads\n";

    std::vector<Parameter> P;
    set_parameters(P);

    long double k = 0.93L;
    spsa_tune(P, k);

    std::ofstream out("tuning_output.txt", std::ios::app);
    for (auto &p : P) {
        *p.ptr = (int) std::round(p.value);
        out << p.name << " " << *p.ptr << "\n";
    }
}
