/**
 * Moraband, known in antiquity as Korriban, was an 
 * Outer Rim planet that was home to the ancient Sith 
 **/

#include "eval.h"
#include "search.h"
#include "tune.h"
#include "uci.h"
#include "zobrist.h"
#include <iostream>
#include <sstream>
#include <string>

int main(int argc, char *argv[]) {
    std::cout << " ___  ___                _                     _ \n \
|  \\/  |               | |                   | | \n\
 |      | ___  _ __ __ _| |__   __ _ _ __   __| | \n\
 | |\\/| |/ _ \\| '__/ _` | '_ \\ / _` | '_ \\ / _` | \n\
 | |  | | (_) | | | (_| | |_) | (_| | | | | (_| | \n\
 \\_|  |_/\\___/|_|  \\__,_|_.__/ \\__,_|_| |_|\\__,_|"
              << std::endl;


    std::cout << "Moraband, known in antiquity as Korriban, was an\n Outer Rim planet that was home to the ancient Sith\n"
              << std::endl;
    mg_init();
    Zobrist::init();
    bb_init();
    initKingRing();

    if (argc > 1) {
        if (std::string(argv[1]) == "bench") {
            if (argc > 2) {
                bench(std::stoi(argv[2]));
            }
            else {
                bench();
            }
        }
#ifdef TUNE
        else if (std::string(argv[1]) == "tune") {
            if (argc > 2) {
                tune(argv[2]);
            }
        }
#endif
    }
    else {
        uci();
    }
    return 0;
}
