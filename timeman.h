#ifndef TIMEMAN_H
#define TIMEMAN_H

#include <chrono>
#include <sys/time.h>
#include <cstdlib>

const int MIN_SEARCH_TIME = 500; // Absolute minimum time to spend searching
static int MOVE_OVERHEAD = 500;
const int ONE_MINUTE = 60000; // 1000 * 60
const int ONE_HOUR = 3600000; // ONE_MINUTE * 60

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

// https://www.chessprogramming.org/Time_Management
inline int64_t get_search_time(int time_left, int inc, int moves, int moves_to_go) {
	int64_t search_time;
	if (moves_to_go || inc) {
		if (moves_to_go && moves_to_go < 8) {
			search_time = std::min((time_left - MOVE_OVERHEAD) / 2, time_left / (moves_to_go + 12) + inc / 2);
		}
		else {
			search_time = std::min((time_left - MOVE_OVERHEAD) / 4, time_left / 27 + 3 * inc / 4);
		}
	}
	else {
		//search_time = (time_left - MOVE_OVERHEAD) / (moves <= 25 ? 40 - moves : 15) + 3 * inc / 4;
		search_time = (time_left - MOVE_OVERHEAD) / 40;
	}
	return search_time > MIN_SEARCH_TIME ? search_time : MIN_SEARCH_TIME;
}

#endif

///