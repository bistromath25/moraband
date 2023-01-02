#include "uci.h"
//#include "test.h"
//#include "perft.h"
#include "io.h"
#include <fstream>

#define ENGINE_NAME "Moraband"
#define ENGINE_VERSION "0.9"
#define ENGINE_AUTHOR "Brighten Zhang"

// Validate incoming UCI move
Move get_uci_move(std::string & token, State & s) {
	Move m;
	token.erase(std::remove(token.begin(), token.end(), ','),
	token.end());
	MoveList mlist(s);
	while (mlist.size() > 0) {
		m = mlist.pop();
		if (toString(m) == token) {
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
		
		else if (token == "wtime")
			is >> search_info.time[WHITE];
		else if (token == "btime")
			is >> search_info.time[BLACK];
		else if (token == "winc")
			is >> search_info.inc[WHITE];
		else if (token == "binc")
			is >> search_info.inc[BLACK];
		else if (token == "movestogo")
			is >> search_info.moves_to_go;
		else if (token == "depth")
			is >> search_info.depth;
		else if (token == "nodes")
			is >> search_info.max_nodes;
		else if (token == "mate")
			is >> search_info.mate;
		else if (token == "movetime")
			is >> search_info.moveTime;
		else if (token == "infinite")
			search_info.infinite = true;
	}

	search_info.clock.set();
	if (!search_info.moveTime) {
		search_info.moveTime = allocate_time(search_info.time[s.getOurColor()], search_info.inc[s.getOurColor()], history.size() / 2, search_info.moves_to_go);
	}
	
	setup_search(s, search_info);
}

// Set position
void position(std::istringstream & is, State & s) {
	std::string token, fen;
	Move m;

	s = State(START_FEN);
	history.init();
	history.push(std::make_pair(NULL_MOVE, s.getKey()));

	is >> token;
	if (token == "fen") {
		//engine_log << "fen";
		//engine_output += "fen";
		while (is >> token && token != "moves") {
			fen += token + " ";
		}
		//engine_log << fen << "\n";
		//engine_output += fen += "\n";
		s = State(fen);
	}
	else if (token == "startpos") {
		is >> token;
		//engine_output += "startpos";
		//engine_log << "startpos";
	}
	else {
		std::cout << "unknown command\n";
		//engine_log << "unknown command\n";
		return;
	}

	while (is >> token) {
		m = get_uci_move(token, s);
		//engine_log << token;
		if (m == NULL_MOVE) {
			std::cout << "illegal move found: " << token << '\n';
			//engine_log << "illegal move found: " << token << '\n';
			//engine_output += "illegal move found: " + token + "\n";
			return;
		}
		else {
			s.makeMove(m);
			history.push(std::make_pair(m, s.getKey()));
		}
	}
	
	//std::cout << s;
}

// UCI setoption command
void set_option(std::string & name, std::string & value) {
	if (name == "Hash") { // Currently changing hash is the only command available
		ttable.resize(std::stoi(value));
	}
	else if (name == "ClearHash") {
		ttable.clear();
	}
	else if (name == "Move Overhead") {
		MOVE_OVERHEAD = std::stoi(value);
		if (MOVE_OVERHEAD < 0) {
			MOVE_OVERHEAD = 0;
		}
		else if (MOVE_OVERHEAD > 10000) {
			MOVE_OVERHEAD = 10000;
		}
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
		//engine_log << "> " << token << "\n";

		if (token == "quit") {
			//engine_log.close();
			break;
		}
		else if (token == "isready") {
			std::cout << "readyok" << std::endl;
		}
		else if (token == "uci") {
			std::cout << "id name " << ENGINE_NAME << " " << ENGINE_VERSION << "\n"
				<< "id author " << ENGINE_AUTHOR << "\n"
					<< "option name Hash type spin default " << DEFAULT_HASH_SIZE
						<< " min " << MIN_HASH_SIZE
							<< " max " << MAX_HASH_SIZE << "\n"
								<< "option name Move Overhead type spin default 500 min 0 max 10000\n";  
			std::cout << "uciok" << std::endl;
			
			/*
			engine_log << "id name " << ENGINE_NAME << " " << ENGINE_VERSION << "\n"
				<< "id author " << ENGINE_AUTHOR << "\n"
					<< "option name Hash type spin default " << DEFAULT_HASH_SIZE
						<< " min " << MIN_HASH_SIZE
							<< " max " << MAX_HASH_SIZE << "\n";
			engine_log << "uciok" << std::endl;
			*/
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
		else if (token == "eval") {
			Evaluate evaluate(root);
			std::cout << evaluate;
		}
		/*
		else if (token == "test") {
			ccrTest();
		}
		else if (token == "perft") {
			perftTest();
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
				std::cout << toString(move) << "      " << -Evaluate(temp).getScore() << "\n";
			}
			std::cout << "------------------" << std::endl;
		}
		*/
	}
}

// void console() {}

///