/* Moraband, known in antiquity as Korriban, was an Outer Rim planet that was home to the ancient Sith */

#include <iostream>
#include <string>
#include <sstream>
#include "search.h"
#include "zobrist.h"
#include "eval.h"
#include "uci.h"

int main(int argc, char* argv[]) {
	std::cout << " ___  ___                _                     _ \n \
|  \\/  |               | |                   | | \n\
 |      | ___  _ __ __ _| |__   __ _ _ __   __| | \n\
 | |\\/| |/ _ \\| '__/ _` | '_ \\ / _` | '_ \\ / _` | \n\
 | |  | | (_) | | | (_| | |_) | (_| | | | | (_| | \n\
 \\_|  |_/\\___/|_|  \\__,_|_.__/ \\__,_|_| |_|\\__,_|" << std::endl;
	
	
	std::cout << "Moraband, known in antiquity as Korriban, was an\n Outer Rim planet that was home to the ancient Sith\n" << std::endl;
	mg_init();
	Zobrist::init();
	bb_init();
	uci();
    return 0;
} 