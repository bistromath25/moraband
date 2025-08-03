/**
 * Moraband, known in antiquity as Korriban, was an 
 * Outer Rim planet that was home to the ancient Sith 
 **/

#ifndef PV_H
#define PV_H

#include "defs.h"
#include "move.h"
#include "movegen.h"
#include "position.h"
#include <array>

constexpr int MAX_PV_SIZE = ((MAX_PLY * MAX_PLY) + MAX_PLY) / 2;

class Variation {
public:
    Variation();
    int size() const;
    void pushToPv(Move move, U64 key, int ply, int score);
    U64 getPvKey(int ply = 0) const;
    Move getPvMove(int ply = 0) const;
    bool isMate() const;
    int getMateInN() const;
    void clearPv();
    void checkPv(Position &s);
    void printPv();
    int getIndex(int ply); // Triangular index
private:
    std::array<std::pair<Move, U64>, MAX_PV_SIZE> pv; // (move, key)
    bool isMatingLine;
    std::size_t sz;
};

#endif
