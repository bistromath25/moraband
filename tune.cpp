/**
 * Moraband, known in antiquity as Korriban, was an 
 * Outer Rim planet that was home to the ancient Sith 
 **/

#include "tune.h"
#include "defs.h"
#include "eval.h"
#include "search.h"
#include <algorithm>
#include <chrono>
#include <cmath>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <random>
#include <sstream>
#include <string>
#include <thread>
#include <vector>

struct Parameter {
    int *variable;
    int value;
    std::string name;
    bool increasing;
    int stability;
};

struct Input {
    Position s;
    long double result;
};

std::vector<long double> diffs[MAX_THREADS];
std::vector<Input> input;
long double k = 0.93L;

const size_t BATCH_SIZE = 8192;
std::vector<int> current_batch;

std::string format_time(double total_seconds) {
    long long total = static_cast<long long>(total_seconds);
    int hours = total / 3600;
    int minutes = (total % 3600) / 60;
    int seconds = total % 60;

    std::ostringstream oss;
    if (hours > 0) {
        oss << hours << "h ";
    }
    oss << std::setfill('0') << std::setw(2) << minutes << ":"
        << std::setfill('0') << std::setw(2) << seconds;
    return oss.str();
}

void set_parameter(Parameter *p) {
    *p->variable = p->value;
}

void set_material(std::vector<Parameter> &parameters) {
    auto add_param = [&](int &var, const std::string &name) {
        parameters.push_back({&var, var, name, true, 1});
    };

    auto add_score = [&](Score &s, const std::string &name) {
        add_param(s.mg, name + ".mg");
        add_param(s.eg, name + ".eg");
    };

    auto add_score_2d = [&](auto &arr, int n, int m, const std::string &name) {
        for (int i = 0; i < n; ++i) {
            for (int j = 0; j < m; ++j) {
                add_score(arr[i][j],
                          name + "[" + std::to_string(i) + "][" + std::to_string(j) + "]");
            }
        }
    };

    add_score(PAWN_WEIGHT, "PAWN_WEIGHT");
    add_score(KNIGHT_WEIGHT, "KNIGHT_WEIGHT");
    add_score(BISHOP_WEIGHT, "BISHOP_WEIGHT");
    add_score(ROOK_WEIGHT, "ROOK_WEIGHT");
    add_score(QUEEN_WEIGHT, "QUEEN_WEIGHT");

    add_score(BISHOP_PAIR, "BISHOP_PAIR");
    add_score(BAD_BISHOP, "BAD_BISHOP");
    add_score(ROOK_OPEN_FILE, "ROOK_OPEN_FILE");
    add_score(ROOK_ON_SEVENTH_RANK, "ROOK_ON_SEVENTH_RANK");
    add_score(KNIGHT_OUTPOST, "KNIGHT_OUTPOST");
    add_score(BISHOP_OUTPOST, "BISHOP_OUTPOST");

    add_score_2d(PAWN_PASSED, 4, 7, "PAWN_PASSED");

    add_score(PAWN_PASSED_CANDIDATE, "PAWN_PASSED_CANDIDATE");
    add_score(PAWN_CONNECTED, "PAWN_CONNECTED");
    add_score(PAWN_ISOLATED, "PAWN_ISOLATED");
    add_score(PAWN_DOUBLED, "PAWN_DOUBLED");
    add_score(PAWN_FULL_BACKWARDS, "PAWN_FULL_BACKWARDS");
    add_score(PAWN_BACKWARDS, "PAWN_BACKWARDS");
    add_score(PAWN_SHIELD_CLOSE, "PAWN_SHIELD_CLOSE");
    add_score(PAWN_SHIELD_FAR, "PAWN_SHIELD_FAR");
    add_score(PAWN_SHIELD_MISSING, "PAWN_SHIELD_MISSING");
    add_score(STRONG_PAWN_ATTACK, "STRONG_PAWN_ATTACK");
    add_score(WEAK_PAWN_ATTACK, "WEAK_PAWN_ATTACK");
    add_score(HANGING, "HANGING");
    add_score(KNIGHT_PAWN_PENALTY, "KNIGHT_PAWN_PENALTY");
    add_score(ROOK_PAWN_BONUS, "ROOK_PAWN_BONUS");
}

void set_mobility(std::vector<Parameter> &parameters) {
    auto add_param = [&](int &var, const std::string &name) {
        parameters.push_back({&var, var, name, true, 1});
    };

    auto add_score = [&](Score &s, const std::string &name) {
        add_param(s.mg, name + ".mg");
        add_param(s.eg, name + ".eg");
    };

    auto add_score_array = [&](auto &arr, int size, const std::string &name) {
        for (int i = 0; i < size; ++i) {
            add_score(arr[i], name + "[" + std::to_string(i) + "]");
        }
    };

    add_score_array(KNIGHT_MOBILITY, 9, "KNIGHT_MOBILITY");
    add_score_array(BISHOP_MOBILITY, 14, "BISHOP_MOBILITY");
    add_score_array(ROOK_MOBILITY, 15, "ROOK_MOBILITY");
    add_score_array(QUEEN_MOBILITY, 28, "QUEEN_MOBILITY");
}

void dump_tuned_values(std::ostream &os) {
    auto print_score = [&](const std::string &name, const Score &s) {
        os << "Score " << name << " = S(" << s.mg << ", " << s.eg << ");\n";
    };

    auto print_array = [&](auto &arr, int size, const std::string &name) {
        os << "Score " << name << "[" << size << "] = {\n    ";
        for (int i = 0; i < size; ++i) {
            os << "S(" << arr[i].mg << ", " << arr[i].eg << ")";
            if (i < size - 1) os << ", ";
        }
        os << "};\n\n";
    };

    auto print_array_2d = [&](auto &arr, int n, int m, const std::string &name) {
        os << "Score " << name << "[" << n << "][" << m << "] = {\n";
        for (int i = 0; i < n; ++i) {
            os << "    {";
            for (int j = 0; j < m; ++j) {
                os << "S(" << arr[i][j].mg << ", " << arr[i][j].eg << ")";
                if (j < m - 1) os << ", ";
            }
            os << "}" << (i < n - 1 ? ",\n" : "\n");
        }
        os << "};\n\n";
    };

    print_score("PAWN_WEIGHT", PAWN_WEIGHT);
    print_score("KNIGHT_WEIGHT", KNIGHT_WEIGHT);
    print_score("BISHOP_WEIGHT", BISHOP_WEIGHT);
    print_score("ROOK_WEIGHT", ROOK_WEIGHT);
    print_score("QUEEN_WEIGHT", QUEEN_WEIGHT);

    print_array(KNIGHT_MOBILITY, 9, "KNIGHT_MOBILITY");
    print_array(BISHOP_MOBILITY, 14, "BISHOP_MOBILITY");
    print_array(ROOK_MOBILITY, 15, "ROOK_MOBILITY");
    print_array(QUEEN_MOBILITY, 28, "QUEEN_MOBILITY");

    print_array_2d(PAWN_PASSED, 4, 7, "PAWN_PASSED");

    print_score("PAWN_PASSED_CANDIDATE", PAWN_PASSED_CANDIDATE);
    print_score("PAWN_CONNECTED", PAWN_CONNECTED);
    print_score("PAWN_ISOLATED", PAWN_ISOLATED);
    print_score("PAWN_DOUBLED", PAWN_DOUBLED);
    print_score("PAWN_FULL_BACKWARDS", PAWN_FULL_BACKWARDS);
    print_score("PAWN_BACKWARDS", PAWN_BACKWARDS);
    print_score("PAWN_SHIELD_CLOSE", PAWN_SHIELD_CLOSE);
    print_score("PAWN_SHIELD_FAR", PAWN_SHIELD_FAR);
    print_score("PAWN_SHIELD_MISSING", PAWN_SHIELD_MISSING);

    print_score("STRONG_PAWN_ATTACK", STRONG_PAWN_ATTACK);
    print_score("WEAK_PAWN_ATTACK", WEAK_PAWN_ATTACK);
    print_score("HANGING", HANGING);

    print_score("KNIGHT_PAWN_PENALTY", KNIGHT_PAWN_PENALTY);
    print_score("ROOK_PAWN_BONUS", ROOK_PAWN_BONUS);

    print_score("BISHOP_PAIR", BISHOP_PAIR);
    print_score("BAD_BISHOP", BAD_BISHOP);
    print_score("ROOK_OPEN_FILE", ROOK_OPEN_FILE);
    print_score("ROOK_ON_SEVENTH_RANK", ROOK_ON_SEVENTH_RANK);
    print_score("KNIGHT_OUTPOST", KNIGHT_OUTPOST);
    print_score("BISHOP_OUTPOST", BISHOP_OUTPOST);
}

void get_fen_info(std::string &s, std::vector<std::string> &v) {
    auto i = s.find(";");
    if (i != std::string::npos) {
        v.push_back(s.substr(0, i));
        v.push_back(s.substr(i + 1));
    }
}

inline long double sigmoid(long double x) {
    long double scale = (k * M_LN10) / 400.0L;
    return 1.0L / (1.0L + expl(-scale * x));
}

inline long double kahan_sum() {
    long double result = 0.0L, c = 0.0L, y, t;
    for (int thread_id = 0; thread_id < MAX_THREADS; ++thread_id) {
        for (int i = 0; i < int(diffs[thread_id].size()); ++i) {
            y = diffs[thread_id][i] - c;
            t = result + y;
            c = (t - result) - y;
            result = t;
        }
    }
    return result;
}

void get_single_error_batch(int thread_id) {
    int actual_batch_size = current_batch.size();
    for (int i = thread_id; i < actual_batch_size; i += MAX_THREADS) {
        int idx = current_batch[i];

        if (input[idx].s.inCheck()) {
            continue;
        }

        SearchInfo si;
        si.infinite = true;
        global_info[thread_id].clear();

        int q = qsearch(input[idx].s, si, global_info[thread_id], 0, NEG_INF, POS_INF);
        if (input[idx].s.getOurColor() == Color::BLACK) {
            q = -q;
        }

        long double p = sigmoid((long double) q);
        diffs[thread_id].push_back((input[idx].result - p) * (input[idx].result - p));
    }
}

void sample_batch() {
    static std::mt19937 rng(std::random_device{}());
    std::uniform_int_distribution<int> dist(0, input.size() - 1);

    current_batch.clear();
    size_t actual_batch_size = std::min(BATCH_SIZE, input.size());
    current_batch.reserve(actual_batch_size);

    for (size_t i = 0; i < actual_batch_size; ++i) {
        current_batch.push_back(dist(rng));
    }
}

long double get_error_on_batch(std::vector<Parameter> &parameters) {
    for (int i = 0; i < int(parameters.size()); ++i) {
        Parameter *p = &parameters[i];
        set_parameter(p);
    }

    std::vector<std::thread> threads;
    for (int i = 0; i < MAX_THREADS; ++i) {
        diffs[i].clear();
        diffs[i].reserve((current_batch.size() / MAX_THREADS) + 1);
        threads.push_back(std::thread(get_single_error_batch, i));
    }

    U64 t = 0;
    for (int i = 0; i < MAX_THREADS; ++i) {
        threads[i].join();
        t += diffs[i].size();
    }

    if (t == 0) {
        return 1.0L;
    }

    return kahan_sum() / ((long double) t);
}

void get_single_error_full(int thread_id) {
    for (size_t i = thread_id; i < input.size(); i += MAX_THREADS) {
        if (input[i].s.inCheck()) {
            continue;
        }
        SearchInfo si;
        si.infinite = true;
        global_info[thread_id].clear();

        int q = qsearch(input[i].s, si, global_info[thread_id], 0, NEG_INF, POS_INF);
        if (input[i].s.getOurColor() == Color::BLACK) {
            q = -q;
        }

        long double p = sigmoid((long double) q);
        diffs[thread_id].push_back((input[i].result - p) * (input[i].result - p));
    }
}

long double get_error_full(std::vector<Parameter> &parameters) {
    for (int i = 0; i < int(parameters.size()); ++i) {
        Parameter *p = &parameters[i];
        set_parameter(p);
    }

    std::vector<std::thread> threads;
    for (int i = 0; i < MAX_THREADS; ++i) {
        diffs[i].clear();
        diffs[i].reserve((input.size() / MAX_THREADS) + 1);
        threads.push_back(std::thread(get_single_error_full, i));
    }

    U64 t = 0;
    for (int i = 0; i < MAX_THREADS; ++i) {
        threads[i].join();
        t += diffs[i].size();
    }

    if (t == 0) return 1.0L;

    return kahan_sum() / ((long double) t);
}

void get_best_k(std::vector<Parameter> &parameters) {
    int min = 60, max = 150;
    k = ((long double) min) / 100.0L;
    long double min_error = get_error_full(parameters);
    std::cerr << "k[" << min << "] " << min_error << "\n";
    k = ((long double) max) / 100.0L;
    long double max_error = get_error_full(parameters);
    std::cerr << "k[" << max << "] " << max_error << "\n";
    while (min < max) {
        if (min_error < max_error) {
            if (min == max - 1) {
                k = ((long double) min) / 100.0L;
                return;
            }
            else {
                max = min + (max - min) / 2;
                k = ((long double) max) / 100.0L;
                max_error = get_error_full(parameters);
                std::cerr << "k[" << max << "] " << max_error << "\n";
            }
        }
        else {
            if (min == max - 1) {
                k = ((long double) max) / 100.0L;
                return;
            }
            else {
                min = min + (max - min) / 2;
                k = ((long double) min) / 100.0L;
                min_error = get_error_full(parameters);
                std::cerr << "k[" << min << "] " << min_error << "\n";
            }
        }
    }
}

void tune_spsa(std::vector<Parameter> &parameters, int iterations) {
    std::mt19937 rng(std::random_device{}());
    std::uniform_int_distribution<int> dist(0, 1);

    double a = 5000.0;
    double c = 2.0;
    double A = iterations / 10.0;
    double alpha = 0.602;
    double gamma = 0.101;

    std::vector<double> current_values(parameters.size());
    for (size_t i = 0; i < parameters.size(); ++i) {
        current_values[i] = parameters[i].value;
    }

    int print_every = std::max(1, iterations / 20);
    auto start_time = std::chrono::steady_clock::now();
    for (int iter = 1; iter <= iterations; ++iter) {
        double a_k = a / std::pow(iter + A, alpha);
        double c_k = c / std::pow(iter, gamma);

        std::vector<double> delta(parameters.size());
        std::vector<Parameter> params_plus = parameters;
        std::vector<Parameter> params_minus = parameters;

        int int_step = std::max(1, (int) std::round(c_k));

        for (size_t i = 0; i < parameters.size(); ++i) {
            delta[i] = dist(rng) ? 1.0 : -1.0;

            int base_val = std::round(current_values[i]);
            params_plus[i].value = base_val + (int_step * (int) delta[i]);
            params_minus[i].value = base_val - (int_step * (int) delta[i]);
        }

        sample_batch();
        long double error_plus = get_error_on_batch(params_plus);
        long double error_minus = get_error_on_batch(params_minus);

        double gradient_multiplier = (error_plus - error_minus) / (2.0 * int_step);

        for (size_t i = 0; i < parameters.size(); ++i) {
            double g_i = gradient_multiplier / delta[i];
            current_values[i] -= a_k * g_i;
            parameters[i].value = std::round(current_values[i]);
        }

        if (iter % print_every == 0 || iter == 1) {
            auto current_time = std::chrono::steady_clock::now();
            std::chrono::duration<double> elapsed = current_time - start_time;

            int iters_left = iterations - iter;
            double eta_seconds = (elapsed.count() / iter) * iters_left;

            std::cerr << "Iteration " << iter << "/" << iterations
                      << " - Batch Error: " << std::min(error_plus, error_minus)
                      << " - ETA: " << format_time(eta_seconds) << "\n";
        }
    }
}

void tune(const std::string &fens_file) {
    std::ifstream fens(fens_file);
    std::string line;
    while (std::getline(fens, line)) {
        std::vector<std::string> info;
        get_fen_info(line, info);

        Input e;
        e.s = Position(info[0]);

        if (info[1] == "1-0") {
            e.result = 1.0L;
        }
        else if (info[1] == "0-1") {
            e.result = 0.0L;
        }
        else if (info[1] == "1/2-1/2") {
            e.result = 0.5L;
        }
        else {
            e.result = -1.0L;
            std::cerr << "invalid fen result " << info[0] << "; " << info[1] << "\n";
            exit(1);
        }

        input.push_back(e);
    }
    std::cerr << "Read " << input.size() << " fens\n";
    fens.close();

    std::cerr << "Tuning with " << MAX_THREADS << " threads\n";
    std::cerr << "Using Mini-Batch Size: " << std::min((size_t) BATCH_SIZE, input.size()) << "\n";

    std::vector<Parameter> best;
    set_material(best);
    set_mobility(best);

    get_best_k(best);
    std::cerr << "k best " << k << "\n";

    int num_iterations = 5000;
    tune_spsa(best, num_iterations);

    std::ofstream tuning_log("tuning_log", std::ios_base::app);
    dump_tuned_values(tuning_log);
    tuning_log.close();

    std::cerr << "Tuning Complete" << std::endl;
}
