#ifndef UCI_H
#define UCI_H

#include <iostream>
#include <string>
#include <sstream>
#include "state.h"
#include "tt.h"
#include "defs.h"
#include "movegen.h"
#include "search.h"
#include "timer.h"

const std::string START_FEN = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq";

void uci();
// void console();

#endif

///