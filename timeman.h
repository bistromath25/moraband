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
/*
inline int64_t get_search_time(int time_left, int inc, int moves, int moves_to_go, int phase) {
	int64_t search_time;
	
	if (moves_to_go) {
		search_time = (time_left - MOVE_OVERHEAD) / moves_to_go;
	}
	else {
		if (inc) {
			search_time = (time_left - MOVE_OVERHEAD) / std::max(10, 40 - moves);
		}
		else {
			search_time = (time_left - MOVE_OVERHEAD) / 40;
		}
	}
	
	search_time += 3 * (inc) / 4;
	search_time += search_time / 100 * (40 - rand() % 80);
	return search_time > MIN_SEARCH_TIME ? search_time : MIN_SEARCH_TIME;
}
*/

inline int64_t get_search_time(int time_left, int inc, int moves, int moves_to_go) {
	
	/*
	int noise = rand() % 8;
	int m = moves >= 16 ? (16 + noise) : moves;
	int factor = 2 - m / (16 + noise);
	int target = (time_left - MOVE_OVERHEAD) / (moves_to_go ? moves_to_go : 40);
	int64_t search_time = factor * target;
	*/
	
	int64_t search_time;
	if (moves_to_go) {
		search_time = (time_left - MOVE_OVERHEAD) / moves_to_go;
	}
	else {
		search_time = time_left / (moves <= 25 ? 40 - moves : 15) + 3 * inc / 4;
	}
	return search_time > MIN_SEARCH_TIME ? search_time : MIN_SEARCH_TIME;
}

#endif

///