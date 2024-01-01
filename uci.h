/**
 * Moraband, known in antiquity as Korriban, was an 
 * Outer Rim planet that was home to the ancient Sith 
 **/

#ifndef UCI_H
#define UCI_H

#include <iostream>
#include <string>
#include <sstream>
#include <utility>
#include "position.h"
#include "tt.h"
#include "defs.h"
#include "movegen.h"
#include "search.h"
#include "timeman.h"

#define ENGINE_NAME "Moraband"
#define ENGINE_VERSION "1.1.2"
#define ENGINE_AUTHOR "Brighten Zhang"

extern int HASH_SIZE;
extern int NUM_THREADS;
extern int MOVE_OVERHEAD;
extern int CONTEMPT;

void uci();

#endif
