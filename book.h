#ifndef BOOK_H
#define BOOK_H

#include "state.h"
#include "defs.h"

#include <iostream>
#include <fstream>
#include <algorithm>
#include <vector>
#include <string>
#include <random>

//extern bool USE_BOOK;
static std::string BOOK_PATH_WHITE = "white.txt";
static std::string BOOK_PATH_BLACK = "black.txt";

static inline int randint(int a, int b) {
	static bool first = true;
	if (first) {
		std::srand(time(0));
		first = false;
	}
	return a + std::rand() % (b + 1 - a);
}

static std::vector<std::string> split(std::string s) {
	std::vector<std::string> res;
	std::string w = "";
	for (auto &x : s) {
		if (x == ' ') {
			res.push_back(w);
			w = "";
		}
		else {
			w += x;
		}
	}
	if (w != "") {
		res.push_back(w);
	}
	return res;
}

static std::string parseFen(std::string ln) {
	std::vector<std::string> v = split(ln);
	return v[0] + ' ' + v[1] + ' ' + v[2] + ' ' + v[3];
}

static std::string getBookMove(std::string gameFen, int c) {
	std::fstream f(c == WHITE ? BOOK_PATH_WHITE : BOOK_PATH_BLACK, std::ios::in);
	if (f.is_open()) {
		std::string ln;
		while (std::getline(f, ln)) {
			if (ln.empty()) {
				break;
			}
			std::vector<std::string> v = split(ln);
			std::string fen = v[0] + ' ' + v[1] + ' ' + v[2] + ' ' + v[3];
			if (fen == gameFen) {
				std::srand(std::time(0));
				return v[randint(7, std::max(7, int(v.size() - 1)))];
			}
		}
	}
	f.close();
	return "none";
}

/*
class Book {
public:
	Book() {
		mBookFile = "";
		entries = 0;
	}
	void load(std::string bookFile) {
		mBookFile = bookFile;
		std::fstream f(mBookFile, std::ios::in);
		if (f.is_open()) {
			std::string ln;
			while (std::getline(f, ln)) {
				if (ln.empty()) {
					break;
				}
				++entries;
			}
			f.close();
		}
		else {
			std::cout << "Book initialization failed, check if " << bookFile << " exists" << std::endl;
		}
	}
	std::string getMove(std::string gameFen) {
		std::fstream f(mBookFile);
		if (f.is_open()) {
			std::string ln;
			while (std::getline(f, ln)) {
				std::vector<std::string> v = split(ln);
				std::string fen = v[0] + ' ' + v[1] + ' ' + v[2] + ' ' + v[3];
				if (fen == gameFen) {
					std::srand(std::time(0));
					return v[randint(7, std::max(7, int(v.size() - 1)))];
				}
			}
		}
		f.close();
		return "none";
	}
	friend std::ostream & operator << (std::ostream & os, const Book & book);
	
private:
	int entries;
	std::string mBookFile;
};

std::ostream & operator << (std::ostream & os, const Book & book) {
	os << "File: " << book.mBookFile << "\nEntries: " << book.entries;
	return os;
}

//extern Book bookWhite, bookBlack;
*/

#endif 

///