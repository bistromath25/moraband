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

#define ENGINE_NAME "Moraband"
#define ENGINE_VERSION "1.0"
#define ENGINE_AUTHOR "Brighten Zhang"

const std::string START_FEN = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq";

void uci();
// void console();

#endif

///