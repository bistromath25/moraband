#include "uci.h"
#include "perft.h"
#include "io.h"
//#include "book.h". // include book.h
#include <fstream>

#define ENGINE_NAME "Moraband"
#define ENGINE_VERSION "0.9.1"
#define ENGINE_AUTHOR "Brighten Zhang"

//Book bookWhite, bookBlack; // define white and black books

/* 
position fen 7K/8/k1P5/7p/8/8/8/8 w - - 0 1
go
...
info depth 21 score cp 0 time 467 nodes 2103985 nps 4495000 pv h8g7 a6b6 g7f6 h5h4 f6e5 b6c6 e5f4 h4h3 f4g3 h3h2 g3g2 h2h1q g2h1 
bestmove h8g7
*/

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
		}
		else if (token == "nodes") {
			is >> search_info.max_nodes;
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
	if (!search_info.moveTime) {
		search_info.moveTime = allocate_time(search_info.time[s.getOurColor()], search_info.inc[s.getOurColor()], history.size() / 2, search_info.moves_to_go);
	}
	
	/* TODO: update (updated search return type)
	if (USE_BOOK) {
		std::string fen = s.getFen();
		std::string bookmove = s.getOurColor() == WHITE ? bookWhite.getMove(fen) : bookBlack.getMove(fen);
		if (bookmove != "none") {
			std::cout << "found bookmove\nbestmove " << bookmove << std::endl;
		}
		else {
			std::cout << "no bookmove found\n";
			setup_search(s, search_info);
		}
	}
	else {
		setup_search(s, search_info);
	}
	*/
	
	m = search(s, search_info);
	std::cout << "bestmove " << to_string(m) << std::endl;
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
			//s.updateGameMoves(token);
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
		D(std::cout << "Move Overhead set to value " << MOVE_OVERHEAD << std::endl);
	}
	/*
	else if (name == "Book") {
		USE_BOOK = value == "true";
		D(std::cout << "USE_BOOK set to value " << USE_BOOK << std::endl;);
		if (USE_BOOK) {
			std::cout << "Loading books...\n";
			bookWhite.load(BOOK_PATH_WHITE);
			bookBlack.load(BOOK_PATH_BLACK);
			std::cout << "White\n" << bookWhite << '\n';
			std::cout << "Black\n" << bookBlack << '\n';
			std::cout << "Books loaded" << std::endl;
		}
	}
	*/
	
	
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
		else if (token == "fen") {
			std::cout << root.getFen() << std::endl;
		}
		else if (token == "eval") {
			Evaluate evaluate(root);
			std::cout << evaluate;
		}
		else if (token == "perft") {
			is >> token;
			perftTest(root, std::stoi(token));
		}
		/*
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
		*/
	}
}

// void console() {}

///