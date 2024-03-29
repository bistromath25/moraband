/**
 * Moraband, known in antiquity as Korriban, was an 
 * Outer Rim planet that was home to the ancient Sith 
 **/

#ifndef ZOBRIST_H
#define ZOBRIST_H

#include "board.h"
#include "defs.h"
#include <cstdlib>
#include <iostream>

namespace Zobrist {
    extern U64 piece_rand[PLAYER_SIZE][PIECE_TYPES_SIZE][BOARD_SIZE];
    extern U64 ep_file_rand[8];
    extern U64 castle_rand[16];
    extern U64 side_to_move_rand;

    void init();

    // Side to move
    inline U64 key() {
        return side_to_move_rand;
    }

    // Castling rights
    inline U64 key(int castle) {
        return castle_rand[castle];
    }

    // En-passant file
    inline U64 key(File ep) {
        return ep_file_rand[ep];
    }

    // Remove/add a piece
    inline U64 key(Color c, PieceType p, Square src) {
        return piece_rand[c][p][src];
    }

    // Moving a piece
    inline U64 key(Color c, PieceType p, Square src, Square dst) {
        return piece_rand[c][p][src] ^ piece_rand[c][p][dst];
    }
}; // namespace Zobrist

#endif
