#include <thread>
#include <vector>
#include <condition_variable>
#include <numeric>
#include "perft.h"

// https://www.chessprogramming.org/Perft_Results#Position_2

static History history;

U64 perft(State & s, int depth) { // Total
	int nodes = 0;
	if (s.getFiftyMoveRule() > 99) return nodes;
	MoveList mlist(s);
	if (depth == 1) return mlist.size();
	Move m;
	while (m = mlist.getBestMove()) {
		State c(s);
		c.makeMove(m);
		nodes += perft(c, depth - 1);
	}
	return nodes;
}

bool ready = false;
std::condition_variable cv;
std::mutex qMutex;
std::mutex lock1;
std::mutex lock2;
std::vector<U64> nodeCount;

void test(State s, MoveList* mList, int depth, int id) {
	/*
	{
		std::unique_lock<std::mutex> lk(qMutex);
		cv.wait(lk, [] { return ready; });
	}
	*/
	Move m = mList->getBestMove();
	/*
	{
		std::lock_guard<std::mutex> lk(qMutex);
		m = mList->getBestMove();
	}
	*/

	while (m != NULL_MOVE) {
		State c(s);
		c.makeMove(m);
		int nodes = perft(c, depth - 1);
		m = mList->getBestMove();
		nodeCount.push_back(nodes);
		/*
		{
			std::lock_guard<std::mutex> lk(qMutex);
			nodeCount.push_back(nodes);
			m = mList->getBestMove();
		} 
		*/
	}
}

U64 MTperft(State& s, int depth) {
	unsigned int nThreads = std::thread::hardware_concurrency();
	std::vector<std::thread> threads;
	nodeCount.clear();
	MoveList mlist(s);
	for (unsigned int i = 0; i < nThreads; ++i) {
		threads.push_back(std::thread(test, s, &mlist, depth, i));
	}
	/*
	{
		std::lock_guard<std::mutex> lk(qMutex);
		ready = true;
		cv.notify_all();
	}
	*/
	for (std::thread& thread : threads)	thread.join();
	return std::accumulate(nodeCount.begin(), nodeCount.end(), 0);
}

void perftTest(State& s, int depth, bool mt) {
	U64 nodes = 0;
	double nps, time;
	Clock clock;
	clock.set();
	nodes = mt ? MTperft(s, depth) : perft(s, depth);
	time = clock.elapsed<std::chrono::microseconds>() / static_cast<double>(1000000);
	std::cout << s << std::endl;
	std::cout << s.getFen() << std::endl;
	std::cout << "Nodes: " << nodes << std::endl;
	std::cout << "NPS:   " << U64(static_cast<long double>(nodes) / time) << std::endl;
}
