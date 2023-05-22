/**
 * Moraband, known in antiquity as Korriban, was an 
 * Outer Rim planet that was home to the ancient Sith 
 **/

#ifndef TIMEMAN_H
#define TIMEMAN_H

#include <chrono>
#include <sys/time.h>
#include <cstdlib>
#include <utility>
#include "defs.h"

const int MIN_SEARCH_TIME = 500; // Absolute minimum time to spend searching
static int MOVE_OVERHEAD = 500; // Move overhead
const int ONE_MINUTE = 60000; // 1000 * 60
const int ONE_HOUR = 3600000; // ONE_MINUTE * 60

/* Clock class for time management */
class Clock {
public:
	void set() {
		mTime = std::chrono::high_resolution_clock::now();
	}
	template<typename T> int64_t elapsed() {
		std::chrono::high_resolution_clock::time_point now = std::chrono::high_resolution_clock::now();
		return std::chrono::duration_cast<T>(now - mTime).count();
	}
private:
	std::chrono::high_resolution_clock::time_point mTime;
};

/* Returns the time assigned for the search */
inline U64 get_search_time(int time_left, int inc, int moves, int moves_to_go) {
	U64 search_time = MIN_SEARCH_TIME;
	if (moves_to_go) {
		search_time = (time_left - MOVE_OVERHEAD) / moves_to_go + 3 * inc / 4;
	}
	else { // Sudden death time control
		search_time = (time_left - MOVE_OVERHEAD) / (moves <= 25 ? 40 - moves : 15) + 3 * inc / 4;
	}
	return search_time > MIN_SEARCH_TIME ? search_time : MIN_SEARCH_TIME;
}

#endif