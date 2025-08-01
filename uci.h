/**
 * Moraband, known in antiquity as Korriban, was an 
 * Outer Rim planet that was home to the ancient Sith 
 **/

#ifndef UCI_H
#define UCI_H

#define ENGINE_NAME "Moraband"
#define ENGINE_AUTHOR "Brighten Zhang"
#if defined(TUNE)
#define ENGINE_VERSION "1.2-tune"
#elif defined(USE_NNUE)
#define ENGINE_VERSION "1.2-nnue"
#else
#define ENGINE_VERSION "1.2"
#endif

extern int HASH_SIZE;
extern int NUM_THREADS;
extern int MOVE_OVERHEAD;
extern int CONTEMPT;

void bench(int depth = 16);
void uci();

#endif
