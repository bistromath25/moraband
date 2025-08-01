/**
 * Moraband, known in antiquity as Korriban, was an 
 * Outer Rim planet that was home to the ancient Sith 
 **/

#ifndef TIMEMAN_H
#define TIMEMAN_H

#include "defs.h"
#include <chrono>
#include <cstdlib>
#include <sys/time.h>
#include <utility>

const int MIN_SEARCH_TIME = 500; // Absolute minimum time to spend searching
const int ONE_MINUTE = 60000;    // 1000 * 60
const int ONE_HOUR = 3600000;    // ONE_MINUTE * 60
extern int MOVE_OVERHEAD;

/** Clock class for time management */
class Clock {
public:
    void set() {
        time = std::chrono::high_resolution_clock::now();
    }
    template<typename T>
    int64_t elapsed() {
        std::chrono::high_resolution_clock::time_point now = std::chrono::high_resolution_clock::now();
        return std::chrono::duration_cast<T>(now - time).count();
    }

private:
    std::chrono::high_resolution_clock::time_point time;
};

/** Returns the time assigned for the search */
inline U64 get_search_time(int time_left, int inc, int moves, int moves_to_go, int time_diff) {
    U64 search_time = MIN_SEARCH_TIME;
    if (moves_to_go) {
        search_time = (time_left - MOVE_OVERHEAD) / moves_to_go + 3 * inc / 4;
    }
    else { // Sudden death time control
        if (inc) {
            search_time = (time_left - MOVE_OVERHEAD) / std::max(10, 40 - moves);
            if (time_diff > 4000) {
                search_time += time_diff / 4;
            }
        }
        else {
            search_time = (time_left - MOVE_OVERHEAD) / 40;
        }
    }
    return search_time > MIN_SEARCH_TIME ? search_time : MIN_SEARCH_TIME;
}

#endif
