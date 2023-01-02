/* Moraband, known in antiquity as Korriban, was an Outer Rim planet that was home to the ancient Sith */

#include <iostream>
#include <string>
#include <sstream>
#include "search.h"
#include "zobrist.h"
#include "eval.h"
#include "uci.h"

int main(int argc, char* argv[]) {
	std::cout << "Moraband, known in antiquity as Korriban, was an Outer Rim planet that was home to the ancient Sith" << std::endl;
	mg_init();
	Zobrist::init();
	bb_init();
	uci();
    return 0;
} 