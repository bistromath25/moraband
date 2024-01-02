/**
 * Moraband, known in antiquity as Korriban, was an 
 * Outer Rim planet that was home to the ancient Sith 
 **/

#include "uci.h"
#include "perft.h"
#include "io.h"

int HASH_SIZE = DEFAULT_HASH_SIZE;
int NUM_THREADS = 1;
int MOVE_OVERHEAD = 500;
int CONTEMPT = 0;

// Validate incoming UCI move
Move get_uci_move(std::string &token, Position &s) {
	Move m;
	token.erase(std::remove(token.begin(), token.end(), ','),
	token.end());
	MoveList moveList(s);
	while (moveList.size() > 0) {
		m = moveList.pop();
		if (to_string(m) == token) {
			return m;
		}
		if (moveList.size() == 0) {
			return NULL_MOVE;
		}
	}
	return NULL_MOVE;
}

// UCI go command
void go(std::istringstream &is, Position &s) {
	std::string token;
	SearchInfo search_info;
	Move m;

	while (is >> token) {
		if (token == "wtime") {
			is >> search_info.time[WHITE];
		}
		else if (token == "btime") {
			is >> search_info.time[BLACK];
		}
		else if (token == "winc") {
			is >> search_info.inc[WHITE];
		}
		else if (token == "binc") {
			is >> search_info.inc[BLACK];
		}
		else if (token == "movestogo") {
			is >> search_info.moves_to_go;
		}
		else if (token == "depth") {
			is >> search_info.depth;
			search_info.infinite = true;
		}
		else if (token == "nodes") {
			is >> search_info.max_nodes;
			search_info.infinite = true;
		}
		else if (token == "movetime") {
			is >> search_info.moveTime;
		}
		else if (token == "infinite") {
			search_info.infinite = true;
		}
	}
	
	search_info.clock.set();
	if (search_info.infinite) {
		search_info.moveTime = ONE_HOUR; // Search for one hour in infinite mode
	}
	else if (!search_info.moveTime) {
		search_info.moveTime = get_search_time(search_info.time[s.getOurColor()], search_info.inc[s.getOurColor()], global_info[0].history.size() / 2, search_info.moves_to_go, search_info.time[s.getOurColor()] - search_info.time[s.getTheirColor()]);
	}
	
	m = search(s, search_info);
	std::cout << "bestmove " << to_string(m) << std::endl;
}

// Set position
void position(std::istringstream &is, Position &s) {
	std::string token, fen;
	Move m;

	s = Position(START_FEN);
	for (int i = 0; i < NUM_THREADS; ++i) {
		global_info[i].history.init();
		global_info[i].history.push(std::make_pair(NULL_MOVE, s.getKey()));
		global_info[i].variation.clearPv();
	}

	is >> token;
	if (token == "fen") {
		while (is >> token && token != "moves") {
			fen += token + " ";
		}
		s = Position(fen);
	}
	else if (token == "startpos") {
		is >> token;
	}
	else {
		std::cout << "unknown command\n";
		return;
	}

	while (is >> token) {
		m = get_uci_move(token, s);
		if (m == NULL_MOVE) {
			std::cout << "illegal move found: " << token << '\n';
			return;
		}
		else {
			s.makeMove(m);
			for (int i = 0; i < NUM_THREADS; ++i) {
				global_info[i].history.push(std::make_pair(m, s.getKey()));
			}
		}
	}
}

// UCI setoption command
void set_option(std::string &name, std::string &value) {
	if (name == "Hash") {
		HASH_SIZE = std::stoi(value);
		tt.resize(HASH_SIZE);
	}
	else if (name == "ClearHash") {
		tt.clear();
	}
	else if (name == "Threads") {
		NUM_THREADS = std::stoi(value);
		if (NUM_THREADS < 1) {
			NUM_THREADS = 1;
		}
		else if (NUM_THREADS > MAX_THREADS) {
			NUM_THREADS = MAX_THREADS;
		}
		D(std::cout << "Threads set to value " << NUM_THREADS << std::endl;);
	}
	else if (name == "Move Overhead") {
		MOVE_OVERHEAD = std::stoi(value);
		if (MOVE_OVERHEAD < 0) {
			MOVE_OVERHEAD = 0;
		}
		else if (MOVE_OVERHEAD > 10000) {
			MOVE_OVERHEAD = 10000;
		}
		D(std::cout << "Move Overhead set to value " << MOVE_OVERHEAD << std::endl;);
	}
	else if (name == "Contempt") {
 		CONTEMPT = std::stoi(value);
 		if (CONTEMPT < -100) {
 			CONTEMPT = -100;
 		}
 		else if (CONTEMPT > 100) {
 			CONTEMPT = 100;
 		}
 		D(std::cout << "Contempt set to value " << CONTEMPT << std::endl;);
 	}
	return;
}

// Benchmark
void bench(int depth) {
	static constexpr std::array fens = { // Bit-genie positions
		"r3k2r/2pb1ppp/2pp1q2/p7/1nP1B3/1P2P3/P2N1PPP/R2QK2R w KQkq a6 0 14",
		"4rrk1/2p1b1p1/p1p3q1/4p3/2P2n1p/1P1NR2P/PB3PP1/3R1QK1 b - - 2 24",
		"r3qbrk/6p1/2b2pPp/p3pP1Q/PpPpP2P/3P1B2/2PB3K/R5R1 w - - 16 42",
		"6k1/1R3p2/6p1/2Bp3p/3P2q1/P7/1P2rQ1K/5R2 b - - 4 44",
		"8/8/1p2k1p1/3p3p/1p1P1P1P/1P2PK2/8/8 w - - 3 54",
		"7r/2p3k1/1p1p1qp1/1P1Bp3/p1P2r1P/P7/4R3/Q4RK1 w - - 0 36",
		"r1bq1rk1/pp2b1pp/n1pp1n2/3P1p2/2P1p3/2N1P2N/PP2BPPP/R1BQ1RK1 b - - 2 10",
		"3r3k/2r4p/1p1b3q/p4P2/P2Pp3/1B2P3/3BQ1RP/6K1 w - - 3 87",
		"2r4r/1p4k1/1Pnp4/3Qb1pq/8/4BpPp/5P2/2RR1BK1 w - - 0 42",
		"4q1bk/6b1/7p/p1p4p/PNPpP2P/KN4P1/3Q4/4R3 b - - 0 37",
		"2q3r1/1r2pk2/pp3pp1/2pP3p/P1Pb1BbP/1P4Q1/R3NPP1/4R1K1 w - - 2 34",
		"1r2r2k/1b4q1/pp5p/2pPp1p1/P3Pn2/1P1B1Q1P/2R3P1/4BR1K b - - 1 37",
		"r3kbbr/pp1n1p1P/3ppnp1/q5N1/1P1pP3/P1N1B3/2P1QP2/R3KB1R b KQkq b3 0 17",
		"8/6pk/2b1Rp2/3r4/1R1B2PP/P5K1/8/2r5 b - - 16 42",
		"1r4k1/4ppb1/2n1b1qp/pB4p1/1n1BP1P1/7P/2PNQPK1/3RN3 w - - 8 29",
		"8/p2B4/PkP5/4p1pK/4Pb1p/5P2/8/8 w - - 29 68",
		"3r4/ppq1ppkp/4bnp1/2pN4/2P1P3/1P4P1/PQ3PBP/R4K2 b - - 2 20",
		"5rr1/4n2k/4q2P/P1P2n2/3B1p2/4pP2/2N1P3/1RR1K2Q w - - 1 49",
		"1r5k/2pq2p1/3p3p/p1pP4/4QP2/PP1R3P/6PK/8 w - - 1 51",
		"q5k1/5ppp/1r3bn1/1B6/P1N2P2/BQ2P1P1/5K1P/8 b - - 2 34",
		"r1b2k1r/5n2/p4q2/1ppn1Pp1/3pp1p1/NP2P3/P1PPBK2/1RQN2R1 w - - 0 22",
		"r1bqk2r/pppp1ppp/5n2/4b3/4P3/P1N5/1PP2PPP/R1BQKB1R w KQkq - 0 5",
		"r1bqr1k1/pp1p1ppp/2p5/8/3N1Q2/P2BB3/1PP2PPP/R3K2n b Q - 1 12",
		"r1bq2k1/p4r1p/1pp2pp1/3p4/1P1B3Q/P2B1N2/2P3PP/4R1K1 b - - 2 19",
		"r4qk1/6r1/1p4p1/2ppBbN1/1p5Q/P7/2P3PP/5RK1 w - - 2 25",
		"r7/6k1/1p6/2pp1p2/7Q/8/p1P2K1P/8 w - - 0 32",
		"r3k2r/ppp1pp1p/2nqb1pn/3p4/4P3/2PP4/PP1NBPPP/R2QK1NR w KQkq - 1 5",
		"3r1rk1/1pp1pn1p/p1n1q1p1/3p4/Q3P3/2P5/PP1NBPPP/4RRK1 w - - 0 12",
		"5rk1/1pp1pn1p/p3Brp1/8/1n6/5N2/PP3PPP/2R2RK1 w - - 2 20",
		"8/1p2pk1p/p1p1r1p1/3n4/8/5R2/PP3PPP/4R1K1 b - - 3 27",
		"8/4pk2/1p1r2p1/p1p4p/Pn5P/3R4/1P3PP1/4RK2 w - - 1 33",
		"8/5k2/1pnrp1p1/p1p4p/P6P/4R1PK/1P3P2/4R3 b - - 1 38",
		"8/8/1p1kp1p1/p1pr1n1p/P6P/1R4P1/1P3PK1/1R6 b - - 15 45",
		"8/8/1p1k2p1/p1prp2p/P2n3P/6P1/1P1R1PK1/4R3 b - - 5 49",
		"8/8/1p4p1/p1p2k1p/P2npP1P/4K1P1/1P6/3R4 w - - 6 54",
		"8/8/1p4p1/p1p2k1p/P2n1P1P/4K1P1/1P6/6R1 b - - 6 59",
		"8/5k2/1p4p1/p1pK3p/P2n1P1P/6P1/1P6/4R3 b - - 14 63",
		"8/1R6/1p1K1kp1/p6p/P1p2P1P/6P1/1Pn5/8 w - - 0 67",
		"1rb1rn1k/p3q1bp/2p3p1/2p1p3/2P1P2N/PP1RQNP1/1B3P2/4R1K1 b - - 4 23",
		"4rrk1/pp1n1pp1/q5p1/P1pP4/2n3P1/7P/1P3PB1/R1BQ1RK1 w - - 3 22",
		"r2qr1k1/pb1nbppp/1pn1p3/2ppP3/3P4/2PB1NN1/PP3PPP/R1BQR1K1 w - - 4 12",
		"2r2k2/8/4P1R1/1p6/8/P4K1N/7b/2B5 b - - 0 55",
		"6k1/5pp1/8/2bKP2P/2P5/p4PNb/B7/8 b - - 1 44",
		"2rqr1k1/1p3p1p/p2p2p1/P1nPb3/2B1P3/5P2/1PQ2NPP/R1R4K w - - 3 25",
		"r1b2rk1/p1q1ppbp/6p1/2Q5/8/4BP2/PPP3PP/2KR1B1R b - - 2 14",
		"6r1/5k2/p1b1r2p/1pB1p1p1/1Pp3PP/2P1R1K1/2P2P2/3R4 w - - 1 36",
		"rnbqkb1r/pppppppp/5n2/8/2PP4/8/PP2PPPP/RNBQKBNR b KQkq c3 0 2",
		"2rr2k1/1p4bp/p1q1p1p1/4Pp1n/2PB4/1PN3P1/P3Q2P/2RR2K1 w - f6 0 20",
		"3br1k1/p1pn3p/1p3n2/5pNq/2P1p3/1PN3PP/P2Q1PB1/4R1K1 w - - 0 23",
		"2r2b2/5p2/5k2/p1r1pP2/P2pB3/1P3P2/K1P3R1/7R w - - 23 93" ,
		"8/P6p/2K1q1pk/2Q5/4p3/8/7P/8 w - - 4 44",
		"7k/8/7P/5B2/5K2/8/8/8 b - - 0 175"
	};

	U64 totalNodes = 0;
	SearchInfo search_info;
	search_info.depth = depth;
	search_info.moveTime = ONE_HOUR;
	Clock clock;
	clock.set();
	for (const auto &fen : fens) {
		std::cout << fen << std::endl;
		Position s(fen);
		search_info.totalNodes = 0;
		search_info.clock.set();
		search(s, search_info);
		totalNodes += search_info.totalNodes;
	}

	double time = clock.elapsed<std::chrono::microseconds>() / static_cast<double>(1000000);
	std::cout << "Time:  " << time << std::endl;
	std::cout << "Nodes: " << totalNodes << std::endl;
	std::cout << "NPS:   " << U64(static_cast<long double>(totalNodes) / time) << std::endl;
}

// Main UCI loop
void uci() {
	Position root(START_FEN);
	std::string command, token;
	std::setvbuf(stdin, NULL, _IONBF, 0);

	while (true) {
		std::getline(std::cin, command);
		std::istringstream is(command);
		is >> std::skipws >> token;

		if (token == "quit") {
			break;
		}
		else if (token == "ucinewgame") {
			tt.clear();
			ptable.clear();
			for (int i = 0; i < NUM_THREADS; ++i) {
				global_info[i].history.init();
				global_info[i].variation.clearPv();
			}
			//tt.setAncient();
		}
		else if (token == "isready") {
			std::cout << "readyok" << std::endl;
		}
		else if (token == "uci") {
			std::cout << "id name " << ENGINE_NAME << " " << ENGINE_VERSION << "\n"
				<< "id author " << ENGINE_AUTHOR << "\n"
					<< "option name Hash type spin default " << DEFAULT_HASH_SIZE << " min " << MIN_HASH_SIZE << " max " << MAX_HASH_SIZE << "\n" 
						<< "option name Threads type spin default 1 min 1 max 16\n"
							<< "option name Move Overhead type spin default 500 min 0 max 10000\n"
								<< "option name Contempt type spin default 0 min -100 max 100\n";
			std::cout << "uciok" << std::endl;
		}
		else if (token == "setoption") {
			std::string name, value;
			is >> token;
			
			while (is >> token && token != "value") {
				name += token;
			}
			while (is >> token) {
				value += token;
			}

			set_option(name, value);
		}
		else if (token == "position") {
			position(is, root);
		}
		else if (token == "go") {
			go(is, root);
		}
		else if (token == "display") {
			std::cout << root;
		}
		else if (token == "fen") {
			std::cout << root.getFen() << std::endl;
		}
		else if (token == "eval") {
			Evaluate evaluate(root);
			std::cout << evaluate;
		}
		else if (token == "perft") {
			is >> token;
			perftTest(root, std::stoi(token), false);
		}
		else if (token == "mtperft") {
			is >> token;
			perftTest(root, std::stoi(token), true);
		}
		else if (token == "moves") {
			MoveList moveList(root);
			std::cout << moveList << std::endl;
		}
		else if (token == "bench") {
			is >> token;
			bench(std::stoi(token));
		}
		else {
			std::cout << "unknown command" << std::endl;
		}
	}
}