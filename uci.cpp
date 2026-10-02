/**
 * Moraband, known in antiquity as Korriban, was an 
 * Outer Rim planet that was home to the ancient Sith 
 **/

#include "uci.h"
#include "bench.h"
#include "eval.h"
#include "perft.h"
#include "search.h"
#include "tt.h"
#ifdef TUNE
#include "tune.h"
#endif
#include <algorithm>
#include <array>
#include <sstream>
#include <thread>

int HASH_SIZE = DEFAULT_HASH_SIZE;
int NUM_THREADS = 1;
int MOVE_OVERHEAD = 500;
bool IS_UCI_CHESS960 = false;
const std::string START_FEN = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq 0 1";
std::thread main_search_thread;
SearchInfo main_search_info;

/** Validate incoming UCI move */
Move get_uci_move(std::string &token, Position &s) {
    token.erase(std::remove(token.begin(), token.end(), ','),
                token.end());
    MoveList moveList(s);
    while (moveList.size() > 0) {
        Move m = moveList.pop();
        if (to_string(m) == token) {
            return m;
        }
    }
    return NULL_MOVE;
}

/** UCI go command */
void set_go(std::istringstream &is, Position &s, SearchInfo &si) {
    std::string token;

    while (is >> token) {
        if (token == "wtime") {
            is >> si.time[WHITE];
        }
        else if (token == "btime") {
            is >> si.time[BLACK];
        }
        else if (token == "winc") {
            is >> si.inc[WHITE];
        }
        else if (token == "binc") {
            is >> si.inc[BLACK];
        }
        else if (token == "movestogo") {
            is >> si.movesToGo;
        }
        else if (token == "depth") {
            is >> si.depth;
            si.infinite = true;
        }
        else if (token == "nodes") {
            is >> si.maxNodes;
            si.infinite = true;
        }
        else if (token == "movetime") {
            is >> si.moveTime;
        }
        else if (token == "infinite") {
            si.infinite = true;
        }
    }

    si.clock.set();
    if (si.infinite) {
        si.moveTime = ONE_HOUR; // Search for one hour in infinite mode
    }
    else if (!si.moveTime) {
        si.moveTime = get_search_time(si.time[s.getOurColor()], si.inc[s.getOurColor()], global_info[0].history.size() / 2, si.movesToGo, si.time[s.getOurColor()] - si.time[s.getTheirColor()]);
    }
}

/** Set position */
void set_position(std::istringstream &is, Position &s) {
    std::string token, fen;

    is >> token;
    if (token == "fen") {
        while (is >> token && token != "moves") {
            fen += token + " ";
        }
    }
    else if (token == "startpos") {
        fen = START_FEN;
        is >> token;
    }
    else {
        std::cout << "unknown command\n";
        return;
    }

    s = Position(fen, IS_UCI_CHESS960);
    for (int i = 0; i < NUM_THREADS; ++i) {
        global_info[i].history.push(std::make_pair(NULL_MOVE, s.getKey()));
    }

    while (is >> token) {
        Move m = get_uci_move(token, s);
        if (m == NULL_MOVE) {
            std::cout << "illegal move found: " << token << std::endl;
            return;
        }
        else {
            StateInfo st;
            s.makeMove(m, st);
            for (int i = 0; i < NUM_THREADS; ++i) {
                global_info[i].history.push(std::make_pair(m, s.getKey()));
            }
        }
    }
}

/** UCI setoption command */
void set_option(std::string &name, std::string &value) {
    if (name == "Hash") {
        HASH_SIZE = clamp(std::stoi(value), 1, MAX_HASH_SIZE);
        tt.resize(HASH_SIZE);
    }
    else if (name == "Clear Hash") {
        tt.clear();
        ptable.clear();
    }
    else if (name == "Threads") {
        NUM_THREADS = clamp(std::stoi(value), 1, MAX_THREADS);
    }
    else if (name == "Move Overhead") {
        MOVE_OVERHEAD = clamp(std::stoi(value), 0, 10000);
    }
    else if (name == "UCI_Chess960") {
        IS_UCI_CHESS960 = value == "true";
    }
}

/** Main UCI loop */
void uci() {
    Position root(START_FEN);
    std::string command, token;
    std::setvbuf(stdin, NULL, _IONBF, 0);

    while (true) {
        std::getline(std::cin, command);
        std::istringstream is(command);
        is >> std::skipws >> token;

        if (token == "stop") {
            main_search_info.stopped = true;
        }
        else if (token == "quit") {
            main_search_info.stopped = true;
            main_search_info.quit = true;
            break;
        }
        else if (token == "ucinewgame") {
            tt.clear();
            ptable.clear();
            for (int i = 0; i < NUM_THREADS; ++i) {
                global_info[i].clear();
                global_info[i].history.init();
            }
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
                      << "option name UCI_Chess960 type check default false\n";
            std::cout << "uciok" << std::endl;
        }
        else if (token == "setoption") {
            std::string name, value;
            is >> token;

            while (is >> token && token != "value") {
                name += token;
                name += " ";
            }
            name.pop_back();
            while (is >> token) {
                value += token;
            }
            set_option(name, value);
        }
        else if (token == "position") {
            set_position(is, root);
        }
        else if (token == "go") {
            if (main_search_thread.joinable()) {
                main_search_info.stopped = true;
                main_search_thread.join();
            }
            main_search_info = SearchInfo{};
            set_go(is, root, main_search_info);
            main_search_thread = std::thread(
                [](Position pos) {
                    Move m = search(pos, main_search_info);
                    std::cout << "bestmove " << to_string(m) << std::endl;
                },
                root);
        }
        else if (token == "display") {
            std::cout << root << std::endl;
        }
        else if (token == "fen") {
            std::cout << root.getFen() << std::endl;
        }
        else if (token == "eval") {
            Evaluate evaluate(root);
            std::cout << evaluate << std::endl;
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
            tt.clear();
            ptable.clear();
            bench(std::stoi(token));
        }
#ifdef TUNE
        else if (token == "tune") {
            is >> token;
            tune(token);
        }
#endif
        else {
            std::cout << "unknown command" << std::endl;
        }
    }

    if (main_search_thread.joinable()) {
        main_search_thread.join();
    }
}
