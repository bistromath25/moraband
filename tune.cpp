/**
 * Moraband, known in antiquity as Korriban, was an 
 * Outer Rim planet that was home to the ancient Sith 
 **/

#include "tune.h"
#include "defs.h"
#include "search.h"
#include <cmath>
#include <fstream>
#include <iostream>
#include <string>
#include <thread>
#include <vector>

std::vector<long double> diffs[MAX_THREADS];
std::vector<std::string> input;
long double k = 0.93L;
U64 num_fens;

void set_parameter(Parameter *p) {
    *p->variable = p->value;
}

void set_material(std::vector<Parameter> &parameters) {
    // parameters.push_back({&PAWN_WEIGHT.mg, PAWN_WEIGHT.mg, "PAWN_WEIGHT.mg", true, 1});
    parameters.push_back({&PAWN_WEIGHT.eg, PAWN_WEIGHT.eg, "PAWN_WEIGHT.eg", true, 1});
    parameters.push_back({&KNIGHT_WEIGHT.mg, KNIGHT_WEIGHT.mg, "KNIGHT_WEIGHT.mg", true, 1});
    parameters.push_back({&KNIGHT_WEIGHT.eg, KNIGHT_WEIGHT.eg, "KNIGHT_WEIGHT.eg", true, 1});
    parameters.push_back({&BISHOP_WEIGHT.mg, BISHOP_WEIGHT.mg, "BISHOP_WEIGHT.mg", true, 1});
    parameters.push_back({&BISHOP_WEIGHT.eg, BISHOP_WEIGHT.eg, "BISHOP_WEIGHT.eg", true, 1});
    parameters.push_back({&ROOK_WEIGHT.mg, ROOK_WEIGHT.mg, "ROOK_WEIGHT.mg", true, 1});
    parameters.push_back({&ROOK_WEIGHT.eg, ROOK_WEIGHT.eg, "ROOK_WEIGHT.eg", true, 1});
    parameters.push_back({&QUEEN_WEIGHT.mg, QUEEN_WEIGHT.mg, "QUEEN_WEIGHT.mg", true, 1});
    parameters.push_back({&QUEEN_WEIGHT.eg, QUEEN_WEIGHT.eg, "QUEEN_WEIGHT.eg", true, 1});
    parameters.push_back({&BISHOP_PAIR.mg, BISHOP_PAIR.mg, "BISHOP_PAIR.mg", true, 1});
    parameters.push_back({&BISHOP_PAIR.eg, BISHOP_PAIR.eg, "BISHOP_PAIR.eg", true, 1});
    parameters.push_back({&BAD_BISHOP.mg, BAD_BISHOP.mg, "BAD_BISHOP.mg", true, 1});
    parameters.push_back({&BAD_BISHOP.eg, BAD_BISHOP.eg, "BAD_BISHOP.eg", true, 1});
    parameters.push_back({&ROOK_OPEN_FILE.mg, ROOK_OPEN_FILE.mg, "ROOK_OPEN_FILE.mg", true, 1});
    parameters.push_back({&ROOK_OPEN_FILE.eg, ROOK_OPEN_FILE.eg, "ROOK_OPEN_FILE.eg", true, 1});
    parameters.push_back({&ROOK_ON_SEVENTH_RANK.mg, ROOK_ON_SEVENTH_RANK.mg, "ROOK_ON_SEVENTH_RANK.mg", true, 1});
    parameters.push_back({&ROOK_ON_SEVENTH_RANK.eg, ROOK_ON_SEVENTH_RANK.eg, "ROOK_ON_SEVENTH_RANK.eg", true, 1});
    parameters.push_back({&KNIGHT_OUTPOST.mg, KNIGHT_OUTPOST.mg, "KNIGHT_OUTPOST.mg", true, 1});
    parameters.push_back({&KNIGHT_OUTPOST.eg, KNIGHT_OUTPOST.eg, "KNIGHT_OUTPOST.eg", true, 1});
    parameters.push_back({&BISHOP_OUTPOST.mg, BISHOP_OUTPOST.mg, "BISHOP_OUTPOST.mg", true, 1});
    parameters.push_back({&BISHOP_OUTPOST.eg, BISHOP_OUTPOST.eg, "BISHOP_OUTPOST.eg", true, 1});
    for (int i = 0; i < 4; ++i) {
        for (int j = 0; j < 7; ++j) {
            parameters.push_back({&PAWN_PASSED[i][j].mg, PAWN_PASSED[i][j].mg, "PAWN_PASSED[" + std::to_string(i) + "][" + std::to_string(j) + "].mg", true, 1});
            parameters.push_back({&PAWN_PASSED[i][j].eg, PAWN_PASSED[i][j].eg, "PAWN_PASSED[" + std::to_string(i) + "][" + std::to_string(j) + "].eg", true, 1});
        }
    }
    parameters.push_back({&PAWN_PASSED_CANDIDATE.mg, PAWN_PASSED_CANDIDATE.mg, "PAWN_PASSED_CANDIDATE.mg", true, 1});
    parameters.push_back({&PAWN_PASSED_CANDIDATE.eg, PAWN_PASSED_CANDIDATE.eg, "PAWN_PASSED_CANDIDATE.eg", true, 1});
    parameters.push_back({&PAWN_CONNECTED.mg, PAWN_CONNECTED.mg, "PAWN_CONNECTED.mg", true, 1});
    parameters.push_back({&PAWN_CONNECTED.eg, PAWN_CONNECTED.eg, "PAWN_CONNECTED.eg", true, 1});
    parameters.push_back({&PAWN_ISOLATED.mg, PAWN_ISOLATED.mg, "PAWN_ISOLATED.mg", true, 1});
    parameters.push_back({&PAWN_ISOLATED.eg, PAWN_ISOLATED.eg, "PAWN_ISOLATED.eg", true, 1});
    parameters.push_back({&PAWN_DOUBLED.mg, PAWN_DOUBLED.mg, "PAWN_DOUBLED.mg", true, 1});
    parameters.push_back({&PAWN_DOUBLED.eg, PAWN_DOUBLED.eg, "PAWN_DOUBLED.eg", true, 1});
    parameters.push_back({&PAWN_FULL_BACKWARDS.mg, PAWN_FULL_BACKWARDS.mg, "PAWN_FULL_BACKWARDS.mg", true, 1});
    parameters.push_back({&PAWN_FULL_BACKWARDS.eg, PAWN_FULL_BACKWARDS.eg, "PAWN_FULL_BACKWARDS.eg", true, 1});
    parameters.push_back({&PAWN_BACKWARDS.mg, PAWN_BACKWARDS.mg, "PAWN_BACKWARDS.mg", true, 1});
    parameters.push_back({&PAWN_BACKWARDS.eg, PAWN_BACKWARDS.eg, "PAWN_BACKWARDS.eg", true, 1});
    parameters.push_back({&PAWN_SHIELD_CLOSE.mg, PAWN_SHIELD_CLOSE.mg, "PAWN_SHIELD_CLOSE.mg", true, 1});
    parameters.push_back({&PAWN_SHIELD_CLOSE.eg, PAWN_SHIELD_CLOSE.eg, "PAWN_SHIELD_CLOSE.eg", true, 1});
    parameters.push_back({&PAWN_SHIELD_FAR.mg, PAWN_SHIELD_FAR.mg, "PAWN_SHIELD_FAR.mg", true, 1});
    parameters.push_back({&PAWN_SHIELD_FAR.eg, PAWN_SHIELD_FAR.eg, "PAWN_SHIELD_FAR.eg", true, 1});
    parameters.push_back({&PAWN_SHIELD_MISSING.mg, PAWN_SHIELD_MISSING.mg, "PAWN_SHIELD_MISSING.mg", true, 1});
    parameters.push_back({&PAWN_SHIELD_MISSING.eg, PAWN_SHIELD_MISSING.eg, "PAWN_SHIELD_MISSING.eg", true, 1});
    parameters.push_back({&STRONG_PAWN_ATTACK.mg, STRONG_PAWN_ATTACK.mg, "STRONG_PAWN_ATTACK.mg", true, 1});
    parameters.push_back({&STRONG_PAWN_ATTACK.eg, STRONG_PAWN_ATTACK.eg, "STRONG_PAWN_ATTACK.eg", true, 1});
    parameters.push_back({&WEAK_PAWN_ATTACK.mg, WEAK_PAWN_ATTACK.mg, "WEAK_PAWN_ATTACK.mg", true, 1});
    parameters.push_back({&WEAK_PAWN_ATTACK.eg, WEAK_PAWN_ATTACK.eg, "WEAK_PAWN_ATTACK.eg", true, 1});
    parameters.push_back({&HANGING.mg, HANGING.mg, "HANGING.mg", true, 1});
    parameters.push_back({&HANGING.eg, HANGING.eg, "HANGING.eg", true, 1});
    parameters.push_back({&KNIGHT_PAWN_PENALTY.mg, KNIGHT_PAWN_PENALTY.mg, "KNIGHT_PAWN_PENALTY.mg", true, 1});
    parameters.push_back({&KNIGHT_PAWN_PENALTY.eg, KNIGHT_PAWN_PENALTY.eg, "KNIGHT_PAWN_PENALTY.eg", true, 1});
    parameters.push_back({&ROOK_PAWN_BONUS.mg, ROOK_PAWN_BONUS.mg, "ROOK_PAWN_BONUS.mg", true, 1});
    parameters.push_back({&ROOK_PAWN_BONUS.eg, ROOK_PAWN_BONUS.eg, "ROOK_PAWN_BONUS.eg", true, 1});
}

void set_mobility(std::vector<Parameter> &parameters) {
    for (int i = 0; i < 9; ++i) {
        parameters.push_back({&KNIGHT_MOBILITY[i].mg, KNIGHT_MOBILITY[i].mg, "KNIGHT_MOBILITY[" + std::to_string(i) + "].mg", true, 1});
        parameters.push_back({&KNIGHT_MOBILITY[i].eg, KNIGHT_MOBILITY[i].eg, "KNIGHT_MOBILITY[" + std::to_string(i) + "].eg", true, 1});
    }
    for (int i = 0; i < 14; ++i) {
        parameters.push_back({&BISHOP_MOBILITY[i].mg, BISHOP_MOBILITY[i].mg, "BISHOP_MOBILITY[" + std::to_string(i) + "].mg", true, 1});
        parameters.push_back({&BISHOP_MOBILITY[i].eg, BISHOP_MOBILITY[i].eg, "BISHOP_MOBILITY[" + std::to_string(i) + "].eg", true, 1});
    }
    for (int i = 0; i < 15; ++i) {
        parameters.push_back({&ROOK_MOBILITY[i].mg, ROOK_MOBILITY[i].mg, "ROOK_MOBILITY[" + std::to_string(i) + "].mg", true, 1});
        parameters.push_back({&ROOK_MOBILITY[i].eg, ROOK_MOBILITY[i].eg, "ROOK_MOBILITY[" + std::to_string(i) + "].eg", true, 1});
    }
    for (int i = 0; i < 28; ++i) {
        parameters.push_back({&QUEEN_MOBILITY[i].mg, QUEEN_MOBILITY[i].mg, "QUEEN_MOBILITY[" + std::to_string(i) + "].mg", true, 1});
        parameters.push_back({&QUEEN_MOBILITY[i].eg, QUEEN_MOBILITY[i].eg, "QUEEN_MOBILITY[" + std::to_string(i) + "].eg", true, 1});
    }
}

void get_fen_info(std::string &s, std::vector<std::string> &v) {
    int i = 0;
    for (int j = 0; j < s.length(); ++j) {
        if (s[j] == ';') {
            v.push_back(s.substr(i, j - i));
            i = j + 2;
        }
        if (j == s.length() - 1) {
            v.push_back(s.substr(i));
        }
    }
}

inline long double sigmoid(long double x) {
    return 1.0L / (1.0L + pow(10.0L, -k * x / 400.0L));
}

inline long double kahan_sum() {
    long double result = 0.0L, c = 0.0L, y, t;
    for (int thread_id = 0; thread_id < NUM_THREADS; ++thread_id) {
        for (int i = 0; i < diffs[thread_id].size(); ++i) {
            y = diffs[thread_id][i] - c;
            t = result + y;
            c = t - result - y;
            result = t;
        }
    }
    return result;
}

void get_single_error(int thread_id) {
    for (int i = thread_id; i < num_fens; i += NUM_THREADS) {
        std::vector<std::string> info;
        get_fen_info(input[i], info);

        Position s(info[0]);
        if (s.inCheck()) {
            continue;
        }

        long double result;
        if (info[1] == "1-0") {
            result = 1.0L;
        }
        else if (info[1] == "0-1") {
            result = 0.0L;
        }
        else if (info[1] == "1/2-1/2") {
            result = 0.5L;
        }
        else {
            result = -1.0L;
            std::cerr << "invalid fen result " << info[0] << "; " << info[1] << "\n";
            exit(1);
        }

        SearchInfo si;
        si.infinite = true;
        global_info[thread_id].clear();
        int q = qsearch(s, si, global_info[thread_id], 0, NEG_INF, POS_INF);
        if (s.getOurColor() == Color::BLACK) {
            q = -q;
        }
        diffs[thread_id].push_back(pow(result - sigmoid((long double) q), 2.0L));
    }
}

long double get_error(std::vector<Parameter> &parameters) {
    for (int i = 0; i < parameters.size(); ++i) {
        Parameter *p = &parameters[i];
        set_parameter(p);
    }
    std::vector<std::thread> threads;
    for (int i = 0; i < NUM_THREADS; ++i) {
        diffs[i].clear();
        threads.push_back(std::thread(get_single_error, i));
    }
    U64 t = 0;
    for (int i = 0; i < NUM_THREADS; ++i) {
        threads[i].join();
        t += diffs[i].size();
    }
    return kahan_sum() / ((long double) t);
}

void get_best_k(std::vector<Parameter> &parameters) {
    int min = 60, max = 150;
    k = ((long double) min) / 100.0L;
    long double min_error = get_error(parameters);
    std::cerr << "k[" << min << "] " << min_error << "\n";
    k = ((long double) max) / 100.0L;
    long double max_error = get_error(parameters);
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
                max_error = get_error(parameters);
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
                min_error = get_error(parameters);
                std::cerr << "k[" << min << "] " << min_error << "\n";
            }
        }
    }
}

void tune(std::vector<Parameter> &parameters) {
    for (int i = 0; i < parameters.size(); ++i) {
        Parameter *p = &parameters[i];
        int d = std::max(10, abs(p->value / 5));
        int min = p->value - d, max = p->value + d;

        p->value = min;
        long double min_error = get_error(parameters);
        std::cerr << p->name << " " << p->value << " " << min_error << "\n";
        p->value = max;
        long double max_error = get_error(parameters);
        std::cerr << p->name << " " << p->value << " " << max_error << "\n";

        while (min < max) {
            if (min_error < max_error) {
                if (min == max - 1) {
                    p->value = min;
                    set_parameter(p);
                    std::cerr << p->name << " " << p->value << " best"
                              << "\n";
                    break;
                }
                else {
                    max = min + (max - min) / 2;
                    p->value = max;
                    max_error = get_error(parameters);
                    std::cerr << p->name << " " << p->value << " " << max_error << "\n";
                }
            }
            else {
                if (min == max - 1) {
                    p->value = max;
                    set_parameter(p);
                    std::cerr << p->name << " " << p->value << " best"
                              << "\n";
                    break;
                }
                else {
                    min = min + (max - min) / 2;
                    p->value = max;
                    min_error = get_error(parameters);
                    std::cerr << p->name << " " << p->value << " " << min_error << "\n";
                }
            }
        }
    }
}

void tune(std::string fens_file) {
    tt.clear();

    std::ifstream fens(fens_file);
    std::string line;
    while (std::getline(fens, line)) {
        input.push_back(line);
        ++num_fens;
    }
    std::cerr << "Read " << num_fens << " fens"
              << "\n";
    fens.close();

    std::vector<Parameter> best;
    set_material(best);
    set_mobility(best);

    get_best_k(best);
    std::cerr << "k best " << k << "\n";
    long double best_error = get_error(best);
    std::cerr << "best error " << best_error << "\n";

    NUM_THREADS = std::thread::hardware_concurrency();
    if (NUM_THREADS == 0) {
        NUM_THREADS = 1;
    }
    std::cerr << "Tuning with " << NUM_THREADS << " threads"
              << "\n";

    for (int i = 0; i < 2; ++i) {
        for (int j = 0; j < best.size(); ++j) {
            best[j].stability = 1;
        }

        tune(best);
        bool improving = true;
        while (improving) {
            improving = false;
            for (int b = 0; b < best.size(); ++b) {
                if (best[b].stability >= 3) {
                    continue;
                }

                std::vector<Parameter> cur = best;
                cur[b].value += best[b].increasing ? 1 : -1;
                long double cur_error = get_error(cur);
                if (cur_error < best_error) {
                    best = cur;
                    best_error = cur_error;
                    improving = true;
                    std::cerr << cur[b].name << " " << cur[b].value << " " << cur_error << " best"
                              << "\n";
                    best[b].stability = 1;
                    continue;
                }
                else {
                    std::cerr << cur[b].name << " " << cur[b].value << " " << cur_error << "\n";
                }

                cur[b].value -= best[b].increasing ? 2 : -2;
                cur_error = get_error(cur);
                if (cur_error < best_error) {
                    best = cur;
                    best_error = cur_error;
                    best[b].increasing = !best[b].increasing;
                    improving = true;
                    std::cerr << cur[b].name << " " << cur[b].value << " " << cur_error << " best"
                              << "\n";
                    best[b].stability = 1;
                    continue;
                }
                else {
                    std::cerr << cur[b].name << " " << cur[b].value << " " << cur_error << "\n";
                    ++best[b].stability;
                }
            }

            for (int b = 0; b < best.size(); ++b) {
                Parameter *p = &best[b];
                std::cerr << "best " << p->name << " " << p->value << "\n";
            }
        }
    }

    std::ofstream tuning_log("tuning_log", std::ios_base::app);
    for (int b = 0; b < best.size(); ++b) {
        Parameter *p = &best[b];
        std::cerr << "best " << p->name << " " << p->value << "\n";
        tuning_log << "best " << p->name << " " << p->value << "\n";
    }
    tuning_log.close();
}
