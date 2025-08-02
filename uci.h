/**
 * Moraband, known in antiquity as Korriban, was an 
 * Outer Rim planet that was home to the ancient Sith 
 **/

#ifndef UCI_H
#define UCI_H

#define ENGINE_NAME "Moraband"
#ifdef TUNE
#define ENGINE_VERSION "1.2-tune"
#else
#define ENGINE_VERSION "1.2"
#endif
#define ENGINE_AUTHOR "Brighten Zhang"

extern int HASH_SIZE;
extern int NUM_THREADS;
extern int MOVE_OVERHEAD;
extern int CONTEMPT;

void bench(int depth = 16);
void uci();

#endif
