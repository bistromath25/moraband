/**
 * Moraband, known in antiquity as Korriban, was an 
 * Outer Rim planet that was home to the ancient Sith 
 **/

#include "zobrist.h"

namespace Zobrist {
    U64 piece_rand[PLAYER_SIZE][PIECE_TYPES_SIZE][BOARD_SIZE];
    U64 ep_file_rand[8];
    U64 castle_rand[16];
    U64 side_to_move_rand;

    U64 rand_64() {
        return U64(rand()) << 32 | U64(rand());
    }

    void init() {
        srand(6736199);
        for (PieceType p = PIECETYPE_PAWN; p < PIECETYPE_NONE; ++p) {
            for (Square s = first_sq; s <= last_sq; ++s) {
                piece_rand[WHITE][p][s] = rand_64();
                piece_rand[BLACK][p][s] = rand_64();
            }
        }
        for (File f = file_a; f <= file_f; ++f) {
            ep_file_rand[f] = rand_64();
        }
        for (int i = 0; i < 16; ++i) {
            castle_rand[i] = rand_64();
        }
        side_to_move_rand = rand_64();
    }
}; // namespace Zobrist