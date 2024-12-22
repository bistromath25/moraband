/**
 * Moraband, known in antiquity as Korriban, was an 
 * Outer Rim planet that was home to the ancient Sith 
 **/

#include "search.h"
#include "io.h"
#include <fstream>
#include <string>
#include <thread>

bool THREAD_STOP = false;
std::thread threads[MAX_THREADS];
GlobalInfo global_info[MAX_THREADS];
std::pair<int, bool> results[MAX_THREADS];

inline int value_to_tt(int value, int ply) {
    if (value >= CHECKMATE_BOUND) {
        value += ply;
    }
    else if (value <= -CHECKMATE_BOUND) {
        value -= ply;
    }
    return value;
}

inline int value_from_tt(int value, int ply) {
    if (value >= CHECKMATE_BOUND) {
        value -= ply;
    }
    else if (value <= -CHECKMATE_BOUND) {
        value += ply;
    }
    return value;
}

inline int null_move_pruning_reduction(int depth, int eval, int beta) {
    return std::max(4, 3 + depth / 3) + clamp((eval - beta) / 256, 0, 2);
}

inline int reverse_futility_pruning_margin(int depth, int eval, bool improving) {
    return 100 * depth / (improving + 1);
}

/* Check if search should be stopped */
bool stop_search(SearchInfo &si) {
    // Not enough time left for search
    if (U64(si.clock.elapsed<std::chrono::milliseconds>()) >= si.moveTime) {
        si.quit = true;
        return true;
    }
    // Incoming quit command
    if (input_waiting()) {
        std::string command(get_input());
        if (command == "quit" || command == "stop") {
            si.quit = true;
            THREAD_STOP = true;
            return true;
        }
    }
    return false;
}

/* Quiescence search */
int qsearch(Position &s, SearchInfo &si, GlobalInfo &gi, int ply, int alpha, int beta) {
    si.nodes++;
    assert(ply <= MAX_PLY);

    if (gi.history.isThreefoldRepetition(s) || s.insufficientMaterial() || s.getFiftyMoveRule() > 99) {
        return DRAW; // Game ust be a draw, return
    }

    if (!(si.nodes & 2047) && (si.quit || stop_search(si) || THREAD_STOP)) {
        return 0;
    }

    // Mate distance pruning
    alpha = std::max((-CHECKMATE + ply), alpha);
    beta = std::min((CHECKMATE - ply), beta);
    if (alpha >= beta) {
        return alpha;
    }

    Evaluate evaluate(s);
    int staticEval = evaluate.getScore();
    if (!s.inCheck()) {
        if (staticEval >= beta) {
            return beta;
        }
        if (staticEval > alpha) {
            alpha = staticEval;
        }
    }

    // Probe tt
    int tt_score = NEG_INF;
    int tt_flag = -1;
    Move tt_move = 0;
#ifndef TUNE_H
    TTEntry tt_entry = tt.probe(s.getKey());
    if (tt_entry.getKey() == s.getKey()) {
        tt_move = tt_entry.getMove();
        tt_score = value_from_tt(tt_entry.getScore(), s.getFiftyMoveRule());
        tt_flag = tt_entry.getFlag();
        if (tt_flag == FLAG_EXACT || (tt_flag == FLAG_LOWER && tt_score >= beta) || (tt_flag == FLAG_UPPER && tt_score <= alpha)) {
            return tt_score;
        }
    }
#endif

    // Generate moves and create the movelist.
    MoveList moveList(s, tt_move, &gi.history, ply, true);
    int score = 0;
    int bestScore = staticEval;
    Move m = NULL_MOVE;
    int legalMoves = 0;
    while ((m = moveList.getBestMove())) {
        if (s.givesCheck(m)) {
            continue;
        }

        legalMoves++;

        // Avoid pruning tactical positions
        if (!s.inCheck() && !s.givesCheck(m) && !s.isEnPassant(m) && !isPromotion(m)) {
            int fScore = staticEval + 100 + PIECE_VALUE[s.onSquare(getDst(m))].score();
            if (fScore <= alpha) {
                bestScore = std::max(bestScore, fScore);
                continue;
            }
        }

        Position c(s);
        c.makeMove(m);

        gi.history.push(std::make_pair(m, c.getKey()));
        score = -qsearch(c, si, gi, ply + 1, -beta, -alpha);
        gi.history.pop();
        if (!(si.nodes & 2047) && (si.quit || stop_search(si) || THREAD_STOP)) {
            return 0;
        }

        if (score >= bestScore) {
            bestScore = score;
        }
        if (bestScore > alpha) {
            alpha = bestScore;
            if (bestScore >= beta) {
                return beta;
            }
        }
    }

    if (!legalMoves && s.check()) {
        return -PSEUDO_CHECKMATE; // Viridithas trick
    }
    return alpha;
}

/* Main search */
int search(Position &s, SearchInfo &si, GlobalInfo &gi, int depth, int ply, int alpha, int beta, bool isPv, bool isNull) {
    assert(depth >= 0);

    if (depth == 0 || ply > MAX_PLY) { // Perform qsearch when regular search is completed
        return qsearch(s, si, gi, ply, alpha, beta);
    }

    si.nodes++;
    si.totalNodes++;

    if (gi.history.isThreefoldRepetition(s) || s.insufficientMaterial() || s.getFiftyMoveRule() > 99) {
        return DRAW;
    }

    Evaluate evaluate(s);
    int staticEval = evaluate.getScore();

    if (ply >= MAX_PLY) {
        return staticEval;
    }

    // Mate distance pruning
    alpha = std::max((-CHECKMATE + ply), alpha);
    beta = std::min((CHECKMATE - ply), beta);
    if (alpha >= beta) {
        return alpha;
    }

    if (!(si.nodes & 2047) && (si.quit || stop_search(si) || THREAD_STOP)) {
        return 0;
    }

    // Probe tt
    bool tt_hit = false;
    int tt_score = NEG_INF;
    int tt_flag = -1;
    Move tt_move = 0;
    TTEntry tt_entry = tt.probe(s.getKey());
    if (tt_entry.getKey() == s.getKey()) {
        tt_hit = true;
        tt_move = tt_entry.getMove();
        tt_score = value_from_tt(tt_entry.getScore(), s.getFiftyMoveRule());
        tt_flag = tt_entry.getFlag();
        if (!isPv && tt_entry.getDepth() >= depth) {
            if (tt_flag == FLAG_EXACT || (tt_flag == FLAG_LOWER && tt_score >= beta) || (tt_flag == FLAG_UPPER && tt_score <= alpha)) {
                return tt_score;
            }
        }
    }

    if (!isPv) {                     // Evaluate current position statically if current node is NOT a PV node
        if (tt_flag == FLAG_EXACT) { // Make use of previous TT score
            staticEval = tt_score;
        }
        else {
            if (tt_hit) {
                if ((staticEval < tt_score && tt_flag == FLAG_LOWER) || (staticEval > tt_score && tt_flag == FLAG_UPPER)) {
                    staticEval = tt_score;
                }
            }
        }
    }

    gi.evalHistory[ply] = staticEval;
    bool improving = ply >= 2 && gi.evalHistory[ply - 2] != NEG_INF   ? gi.evalHistory[ply] > gi.evalHistory[ply - 2]
                     : ply >= 4 && gi.evalHistory[ply - 4] != NEG_INF ? gi.evalHistory[ply] > gi.evalHistory[ply - 4]
                                                                      : true;

    Move best_move = tt_move;
    if (gi.variation.getPvKey() == s.getKey()) {
        best_move = gi.variation.getPvMove(ply);
    }

    // Pruning
    if (!isPv && !s.inCheck() && s.getNonPawnPieceCount() && beta > -CHECKMATE_BOUND) {
        // Reverse futility pruning
        if (depth <= REVERSE_FUTILITY_DEPTH && staticEval - reverse_futility_pruning_margin(depth, staticEval, improving) >= beta) {
            return beta;
        }
        // Null move pruning
        // Make a null move and search to a reduced depth
        if (!isNull && depth >= NULL_MOVE_DEPTH && staticEval + NULL_MOVE_MARGIN >= beta) {
            Position n;
            std::memmove(&n, &s, sizeof(s));
            n.makeNull();
            gi.history.push(std::make_pair(NULL_MOVE, n.getKey()));
            int nullScore = -search(n, si, gi, std::max(1, depth - null_move_pruning_reduction(depth, staticEval, beta)), ply + 1, -beta, -beta + 1, false, true);
            gi.history.pop();
            if (nullScore >= beta) {
                if (nullScore >= CHECKMATE_BOUND) {
                    nullScore = beta;
                }
                return nullScore;
            }
        }
        // Razoring
        if (depth <= RAZOR_DEPTH && staticEval <= alpha - RAZOR_MARGIN) {
            if (depth == 1) {
                return qsearch(s, si, gi, 0, alpha, beta);
            }
            int r = alpha - RAZOR_MARGIN;
            int rScore = qsearch(s, si, gi, 0, r, r + 1);
            if (rScore <= r) {
                return rScore;
            }
        }
    }

    // Internal iterative deepening
    // In case no best move was found, perform a shallower search to determine which move to properly seach first
    if (!tt_move && !isNull && !s.inCheck() && (isPv || staticEval + 100 >= beta) && depth >= 5) {
        search(s, si, gi, depth - 2, ply, alpha, beta, isPv, true);
        tt_entry = tt.probe(s.getKey());
        tt_move = tt_entry.getMove();
    }

    MoveList moveList(s, tt_move, &(gi.history), ply);

    int score = 0;
    int bestScore = NEG_INF;

    int legalMoves = 0;
    int oldAlpha = alpha;
    Move m;
    int d;
    while ((m = moveList.getBestMove())) {
        d = depth - 1; // Early pruning
        legalMoves++;

        if (bestScore > -CHECKMATE_BOUND && legalMoves > 1 && !s.inCheck() && !s.givesCheck(m) && !isPromotion(m) && s.getNonPawnPieceCount()) {
            // Futility pruning
            if (depth <= FUTILITY_DEPTH && !isPv && staticEval + 100 * d <= alpha) {
                continue;
            }
            // SEE-based pruning (prune if SEE too low)
            // Prune if see(move) < -(pawn * 2 ^ (depth - 1))
            if (depth <= 3 && s.see(m) < -PIECE_VALUE[PIECETYPE_PAWN].score() * (1 << (depth - 1)) && m != tt_move) {
                continue;
            }
            // Late move reduction
            if (depth >= LATE_MOVE_REDUCTION_DEPTH && legalMoves > (isPv ? 5 : 3) + !improving && !s.isCapture(m)) {
                d -= 1 + !isPv + (legalMoves > 8);
                d = std::max(1, d);
            }
        }

        Position c(s);
        c.makeMove(m);                                  // Make move on new position
        gi.history.push(std::make_pair(m, c.getKey())); // Add move to history

        if (c.inCheck() && depth == 1) {
            d++;
        }

        // Search PV move
        if (legalMoves == 1) {
            best_move = m;
            score = -search(c, si, gi, d, ply + 1, -beta, -alpha, isPv, isNull);
        }
        else {
            score = -search(c, si, gi, d, ply + 1, -alpha - 1, -alpha, false, isNull);
            if (score > alpha) {
                score = -search(c, si, gi, std::max(d, depth - 1), ply + 1, -alpha - 1, -alpha, false, isNull);
            }
            if (alpha < score && score < beta) {
                score = -search(c, si, gi, d, ply + 1, -beta, -alpha, true, isNull);
            }
        }

        gi.history.pop(); // Pop move from history

        if (!(si.nodes & 2047) && (si.quit || stop_search(si) || THREAD_STOP)) {
            return 0;
        }

        if (score > bestScore) {
            bestScore = score;
            best_move = m;
        }
        alpha = std::max(alpha, bestScore);
        if (alpha >= beta) {
            alpha = beta;
            if (s.isQuiet(m)) {
                gi.history.update(m, depth, ply, true);
            }
            break;
        }
        else {
            if (s.isQuiet(m)) {
                gi.history.update(m, depth, ply, false);
            }
        }
    }

    if (!legalMoves) { // Either completely lost or (hopefully) a draw
        return s.check() ? -CHECKMATE + ply : STALEMATE;
    }

    if (oldAlpha < alpha && alpha < beta && !si.quit) {
        gi.variation.pushToPv(best_move, s.getKey(), ply, alpha);
    }

    U64 flag = alpha >= beta      ? FLAG_LOWER
               : alpha > oldAlpha ? FLAG_EXACT
                                  : FLAG_UPPER;

    assert(best_move != NULL_MOVE);
    tt.insert(best_move, flag, depth, value_to_tt(alpha, ply), s.getKey());
    return alpha;
}

/* Root search */
int search_root(Position &s, SearchInfo &si, GlobalInfo &gi, int depth, int ply, int alpha, int beta) {
    assert(depth >= 0);

    si.nodes++;
    si.totalNodes++;

    if (!(si.nodes & 2047) && (si.quit || stop_search(si) || THREAD_STOP)) {
        return 0;
    }

    // Probe tt
    Move tt_move = NULL_MOVE;
    TTEntry tt_entry = tt.probe(s.getKey());
    if (tt_entry.getKey() == s.getKey()) {
        tt_move = tt_entry.getMove();
    }

    MoveList moveList(s, tt_move, &(gi.history), ply);

    int score = 0;
    int bestScore = NEG_INF;
    Move best_move = NULL_MOVE;

    int legalMoves = 0;
    int oldAlpha = alpha;
    Move m;
    int d;
    while ((m = moveList.getBestMove())) {
        d = depth - 1;
        legalMoves++;

        Position c(s);
        c.makeMove(m);
        gi.history.push(std::make_pair(m, c.getKey()));

        if (c.inCheck() && depth == 1) {
            d++;
        }

        if (legalMoves == 1) {
            best_move = m;
            score = -search(c, si, gi, d, ply + 1, -beta, -alpha, true, false);
        }
        else {
            score = -search(c, si, gi, d, ply + 1, -alpha - 1, -alpha, false, false);
            if (score > alpha) {
                score = -search(c, si, gi, std::max(d, depth - 1), ply + 1, -alpha - 1, -alpha, false, false);
            }
            if (alpha < score && score < beta) {
                score = -search(c, si, gi, d, ply + 1, -beta, -alpha, true, false);
            }
        }

        gi.history.pop();

        if (!(si.nodes & 2047) && (si.quit || stop_search(si) || THREAD_STOP)) {
            return 0;
        }

        if (score > bestScore) {
            bestScore = score;
            best_move = m;
        }
        alpha = std::max(alpha, bestScore);
        if (alpha >= beta) {
            alpha = beta;
            if (s.isQuiet(m)) {
                gi.history.update(m, depth, ply, true);
            }
            break;
        }
        else {
            if (s.isQuiet(m)) {
                gi.history.update(m, depth, ply, false);
            }
        }
    }

    if (!legalMoves) {
        return s.check() ? -CHECKMATE + ply : STALEMATE;
    }

    if (oldAlpha < alpha && alpha < beta && !si.quit) {
        gi.variation.pushToPv(best_move, s.getKey(), ply, alpha);
    }

    U64 flag = alpha >= beta      ? FLAG_LOWER
               : alpha > oldAlpha ? FLAG_EXACT
                                  : FLAG_UPPER;

    assert(best_move != NULL_MOVE);
    tt.insert(best_move, flag, depth, value_to_tt(alpha, ply), s.getKey());
    return alpha;
}

/* Multi-threaded search driver */
void parallel_search(Position s, SearchInfo si, int depth, int alpha, int beta, int t) {
    auto &[value, valid] = results[t];
    auto &gi = global_info[t];
    valid = false;
    value = search_root(s, si, gi, depth, 0, alpha, beta);
    if (!THREAD_STOP) {
        THREAD_STOP = true;
        valid = true;
    }
}

/* Iterative deepening routine */
Move iterative_deepening(Position &s, SearchInfo &si) {
    Move best_move = NULL_MOVE;
    //int alpha = NEG_INF;
    //int beta = POS_INF;
    int results_index = 0;
    int score = 0;
    for (int d = 1; !si.quit && d < MAX_PLY; ++d) {
        for (int i = 0; i < NUM_THREADS; ++i) {
            global_info[i].variation.clearPv();
        }

        results_index = 0;
        THREAD_STOP = false;
        if (d > 4) {
            for (int i = 1; i < NUM_THREADS; ++i) {
                threads[i] = std::thread{parallel_search, s, si, d + (i & 1), NEG_INF, POS_INF, i};
            }

            results[0].first = search_root(s, si, global_info[0], d, 0, NEG_INF, POS_INF);
            THREAD_STOP = true;

            for (int i = 1; i < NUM_THREADS; ++i) {
                threads[i].join(); // join all threads at end of search
            }
        }
        else {
            results[0].first = search_root(s, si, global_info[0], d, 0, NEG_INF, POS_INF);
        }

        if (si.quit || (d > 1 && stop_search(si))) {
            break;
        }

        score = results[results_index].first;
        global_info[results_index].variation.checkPv(s);

        std::cout << "info depth " << d;
        if (global_info[results_index].variation.isMate()) {
            int n = global_info[results_index].variation.getMateInN();
            std::cout << " score mate " << (score > 0 ? n : -n);
        }
        else {
            std::cout << " score cp " << score;
        }

        std::cout << " time " << si.clock.elapsed<std::chrono::milliseconds>()
                  << " nodes " << si.totalNodes
                  << " nps " << si.totalNodes / (si.clock.elapsed<std::chrono::milliseconds>() + 1) * 1000;
        global_info[results_index].variation.printPv();
        std::cout << std::endl;

        best_move = global_info[results_index].variation.getPvMove();

        if (U64(si.clock.elapsed<std::chrono::milliseconds>()) * 2 > si.moveTime) {
            break; // Insufficient time for next search iteration
        }
        if ((si.depth && d >= si.depth) || (si.max_nodes && si.totalNodes >= U64(si.max_nodes)) || (si.nodes == si.prevNodes)) {
            break;
        }

        si.prevNodes = si.nodes;
        si.nodes = 0;
    }

    return best_move;
}

/* Search driver */
Move search(Position &s, SearchInfo &si) {
    for (int i = 0; i < NUM_THREADS; ++i) {
        global_info[i].clear();
        results[i].first = 0;
        results[i].second = false;
    }
    return iterative_deepening(s, si);
}
