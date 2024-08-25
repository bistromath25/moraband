/**
 * Moraband, known in antiquity as Korriban, was an 
 * Outer Rim planet that was home to the ancient Sith 
 **/

#include <cmath>
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include "defs.h"
#include "search.h"
#include "tune.h"

std::vector<long double> diffs[MAX_THREADS];
std::vector<std::string> input;
long double k = 0.93L;
U64 num_fens;

void set_parameter(Parameter *p) {
    *p->variable = p->value;
}

void set_material(std::vector<Parameter> & parameters) {

}

void get_fen_info(std::string &s, std::vector<std::string> &v) {
    int i = 0;
    for (int j = 0; j < s.length(); ++j) {
        if (s[j] == ';') {
            v.push_back(s.substr(i, j - i));
            i = j + 1;
        }
        if (j == s.length() - 1) {
            v.push_back(s.substr(j));
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

    unsigned long long t = 0;
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
    std::cerr << "k[" << min << "] " << min_error << std::endl;
    k = ((long double) max) / 100.0L;
    long double max_error = get_error(parameters);
    std::cerr << "k[" << max << "] " << max_error << std::endl;
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
                std::cerr << "k[" << max << "] " << max_error << std::endl;
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
                std::cerr << "k[" << min << "] " << min_error << std::endl;
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
        std::cerr << p->name << "[" << p->value << "] " << min_error << std::endl;
        p->value = max;
        long double max_error = get_error(parameters);
        std::cerr << p->name << "[" << p->value << "] " << max_error << std::endl;

        while (min < max) {
            if (min_error < max_error) {
                if (min == max - 1) {
                    p->value = min;
                    set_parameter(p);
                    std::cerr << p->name << "[" << p->value << "] best" << std::endl;
                    break;
                }
                else {
                    max = min + (max - min) / 2;
                    p->value = max;
                    max_error = get_error(parameters);
                    std::cerr << p->name << "[" << p->value << "] " << max_error << std::endl;
                }
            }
            else {
                if (min == max - 1) {
                    p->value = max;
                    set_parameter(p);
                    std::cerr << p->name << "[" << p->value << "] best" << std::endl;
                    break;
                }
                else {
                    min = min + (max - min) / 2;
                    p->value = max;
                    min_error = get_error(parameters);
                    std::cerr << p->name << "[" << p->value << "] " << min_error << std::endl;
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
    std::cerr << "Read " << num_fens << " fens" << std::endl;
    fens.close();

    std::vector<Parameter> best;
    set_material(best);

    get_best_k(best);
    std::cout << "k best " << k << std::endl;
    long double best_error = get_error(best);
    std::cout << "best error " << best_error << std::endl;

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
                    std::cerr << cur[b].name << "[" << cur[b].value << "] " << cur_error << " best" << std::endl;
                    best[b].stability = 1;
                    continue;
                }
                else {
                    std::cerr << cur[b].name << "[" << cur[b].value << "] " << cur_error << std::endl;
                }

                cur[b].value -= best[b].increasing ? 2 : -2;
                cur_error = get_error(best);
                if (cur_error < best_error) {
                    best = cur;
                    best_error = cur_error;
                    best[b].increasing = !best[b].increasing;
                    improving = true;
                    std::cerr << cur[b].name << "[" << cur[b].value << "] " << cur_error << " best" << std::endl;
                    best[b].stability = 1;
                    continue;
                }
                else {
                    std::cerr << cur[b].name << "[" << cur[b].value << "] " << cur_error << std::endl;
                    ++best[b].stability;
                }
            }

            for (int b = 0; b < best.size(); ++b) {
                Parameter *p = &best[b];
                std::cerr << "best " << p->name << " " << p->value << std::endl;
            }
        }
    }

    for (int b = 0; b < best.size(); ++b) {
        Parameter *p = &best[b];
        std::cerr << "best " << p->name << " " << p->value << std::endl;
    }
}
