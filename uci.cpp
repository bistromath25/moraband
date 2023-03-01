#include "uci.h"
#include "perft.h"
#include "io.h"
//#include "book.h". // include book.h
#include <fstream>

//Book bookWhite, bookBlack; // define white and black books

/* 
position fen 7K/8/k1P5/7p/8/8/8/8 w - - 0 1
go
...
info depth 21 score cp 0 time 467 nodes 2103985 nps 4495000 pv h8g7 a6b6 g7f6 h5h4 f6e5 b6c6 e5f4 h4h3 f4g3 h3h2 g3g2 h2h1q g2h1 
bestmove h8g7
*/

int NUM_THREADS = 1;

// Validate incoming UCI move
Move get_uci_move(std::string & token, State & s) {
	Move m;
	token.erase(std::remove(token.begin(), token.end(), ','),
	token.end());
	MoveList mlist(s);
	while (mlist.size() > 0) {
		m = mlist.pop();
		if (to_string(m) == token) {
			return m;
		}
		if (mlist.size() == 0) {
			return NULL_MOVE;
		}
	}
	return NULL_MOVE;
}

void go(std::istringstream & is, State & s) {
	std::string token;
	SearchInfo search_info;
	Move m;

	while (is >> token) {
		if (token == "searchmoves") {
			while (is >> token) {
				m = get_uci_move(token, s);
				if (m != NULL_MOVE) {
					search_info.sm.push_back(m);
				}
				else {
					std::cout << "illegal move found: " << token << '\n';
					return;
				}
			}
		}
		else if (token == "wtime") {
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
		else if (token == "mate") {
			is >> search_info.mate;
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
		search_info.moveTime = get_search_time(search_info.time[s.getOurColor()], search_info.inc[s.getOurColor()], global_info[0].history.size() / 2, search_info.moves_to_go);
	}
	
	m = search(s, search_info, NUM_THREADS); // searching main only
	std::cout << "bestmove " << to_string(m) << std::endl;
}

// Set position
void position(std::istringstream & is, State & s) {
	std::string token, fen;
	Move m;

	s = State(START_FEN);
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
		s = State(fen);
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
			//s.updateGameMoves(token);
			for (int i = 0; i < NUM_THREADS; ++i) {
				global_info[i].history.push(std::make_pair(m, s.getKey()));
			}
		}
	}
	
	//std::cout << s;
}

// UCI setoption command
void set_option(std::string & name, std::string & value) {
	if (name == "Hash") {
		tt.resize(std::stoi(value));
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
			CONTEMPT = 100;
		}
		else if (CONTEMPT > 100) {
			CONTEMPT = -100;
		}
		D(std::cout << "Contempt set to value " << CONTEMPT << std::endl;);
	}
	return;
}

// Main UCI loop
void uci() {
	State root(START_FEN);
	std::string command, token;
	std::setvbuf(stdin, NULL, _IONBF, 0);

	while (true) {
		std::getline(std::cin, command);
		std::istringstream is(command);
		is >> std::skipws >> token;

		if (token == "quit") {
			//engine_log.close();
			break;
		}
		else if (token == "ucinewgame") {
			tt.clear();
			ptable.clear();
		}
		else if (token == "isready") {
			std::cout << "readyok" << std::endl;
		}
		else if (token == "uci") {
			std::cout << "id name " << ENGINE_NAME << " " << ENGINE_VERSION << "\n"
				<< "id author " << ENGINE_AUTHOR << "\n"
					<< "option name Hash type spin default " << DEFAULT_HASH_SIZE << " min " << MIN_HASH_SIZE << " max " << MAX_HASH_SIZE << "\n" 
						<< "option name Threads type spin default 1 max 16 min 1\n"
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
			//std::cout << root.getGamePhase() << std::endl;
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
			MoveList mlist(root);
			std::cout << mlist.size() << " possible moves:\n";
			std::cout << "Move | Static eval\n";
			std::cout << "------------------\n";
			while (mlist.size() >= 1) {
				Move move = mlist.pop();
				State temp = root;
				temp.makeMove(move);
				std::cout << to_string(move) << "      " << -Evaluate(temp).getScore() << "\n";
			}
			std::cout << "------------------" << std::endl;
		}
		else {
			std::cout << "unknown command" << std::endl;
		}
	}
}

// void console() {}

///