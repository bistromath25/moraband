/**
 * Moraband, known in antiquity as Korriban, was an 
 * Outer Rim planet that was home to the ancient Sith 
 **/

#ifndef TUNE_H
#define TUNE_H

#include "defs.h"
#include <string>
#include <vector>

extern std::vector<long double> diffs[MAX_THREADS];
extern std::vector<std::string> input;
extern long double k;
extern U64 num_fens;

struct Parameter {
    int *variable;
    int value;
    std::string name;
    bool increasing;
    int stability;
};

void tune(std::string fen_file);

#endif
