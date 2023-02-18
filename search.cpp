#include "search.h"
#include "io.h"
#include <string>
#include <fstream>
#include <thread>

//Variation variation;
//History history;

inline int value_to_tt(int value, int ply) {
	if (value >= MAX_CHECKMATE) {
		value += ply;
	}
	else if (value <= -MAX_CHECKMATE) {
		value -= ply;
	}
	return value;
}

inline int value_from_tt(int value, int ply) {
	if (value >= MAX_CHECKMATE) {
		value -= ply;
	}
	else if (value <= -MAX_CHECKMATE) {
		value += ply;
	}
	return value;
}

// Check if search should be stopped
bool interrupt(SearchInfo& si) {
	// Not enough time left for search
	if (si.clock.elapsed<std::chrono::milliseconds>() >= si.moveTime && !si.infinite) {
		si.quit = true;
		return true;
	}
	// Incoming quit command
	if (input_waiting()) {
		std::string command(get_input());
		if (command == "quit" || command == "stop") {
			si.quit = true;
			return true;
		}
	}

	return false; // No interrupt
}

int qsearch(State& s, SearchInfo& si, GlobalInfo& gi, int ply, int alpha, int beta) {
	si.nodes++;
	assert(ply < MAX_PLY);

	if (gi.history.isThreefoldRepetition(s) || s.insufficientMaterial() || s.getFiftyMoveRule() > 99) {
		return DRAW; // Game must be a draw, return
	}

	Evaluate evaluate(s);

	int qscore = evaluate.getScore();

	// If a beta cutoff is found, return qscore
	if (qscore >= beta) {
		return beta;
	}

	// Update alpha
	alpha = std::max(alpha, qscore);

	// Generate moves and create the movelist.
	MoveList mlist(s, NULL_MOVE, &gi.history, ply, true);

	int bestScore = NEG_INF;
	int score;
	Move m;

	// Futility pruning: do not search subtrees which are unlikely to improve the alpha value score
	while ((m = mlist.getBestMove())) {
		// Avoid pruning tactical positions
		if (!s.inCheck() && !s.givesCheck(m) && !s.isEnPassant(m) && !isPromotion(m)) {
			int fScore = qscore + 100 + PieceValue[s.onSquare(getDst(m))];
			if (fScore <= alpha) {
				bestScore = std::max(bestScore, fScore);
				continue;
			}
		}

		State c(s);
		c.makeMove(m);

		gi.history.push(std::make_pair(m, c.getKey()));
		score = -qsearch(c, si, gi, ply + 1, -beta, -alpha);
		if (!(si.nodes & 2047) && (si.quit || interrupt(si) || THREAD_STOP)) {
			return 0;
		}

		gi.history.pop();
		if (score >= bestScore) {
			bestScore = score;
		}

		alpha = std::max(alpha, bestScore); // Update alpha value

		if (alpha >= beta) {
			return beta;
		}
	}

	if (bestScore == NEG_INF && s.check()) {
		return -CHECKMATE + ply;
	}

	// Fail-hard alpha beta score
	return alpha;
}

int search(State& s, SearchInfo& si, GlobalInfo& gi, int depth, int ply, int alpha, int beta, bool isPv, bool isNull, bool isRoot) {
	assert(depth >= 0);
	Move best_move = NULL_MOVE;

	si.nodes++; gi.nodes++;

	if (depth == 0) {
		return qsearch(s, si, gi, ply, alpha, beta);
	}
	if (!isRoot && (gi.history.isThreefoldRepetition(s) || s.insufficientMaterial() || s.getFiftyMoveRule() > 99)) {
		return DRAW;
	}
	
	// Mate distance pruning
	alpha = std::max((-CHECKMATE + ply), alpha);
	beta = std::min((CHECKMATE - ply), beta);
	if (alpha >= beta) {
		return alpha;
	}
	
	if (!(si.nodes & 2047) && (si.quit || interrupt(si) || THREAD_STOP)) {
		return 0;
	}

	// Probe tt
	bool tt_hit = false;
	int tt_score = NEG_INF;
	int tt_flag = -1;
	Move tt_move = 0;
	TTEntry tt_entry = tt.probe(s.getKey());
	//D(std::cout << s.getKey() << ' ' << tt_entry.getKey() << std::endl;);
	if (tt_entry.getKey() == s.getKey()) {
		tt_hit = true;
		tt_move = tt_entry.getMove();
		tt_score = value_from_tt(tt_entry.getScore(), s.getFiftyMoveRule());
		tt_flag = tt_entry.getFlag();
		if (!isPv && tt_entry.getDepth() >= depth) {
			if (tt_flag == FLAG_EXACT || (tt_flag == FLAG_LOWER && tt_score >= beta) || (tt_flag == FLAG_UPPER && tt_score <= alpha)) {
				return tt_score; // tt_score
			}
		}
	}

	// Check PV variation
	if (gi.variation.getPvKey(ply) == s.getKey()) {
		best_move = gi.variation.getPvMove(ply);
	}

	int staticEval = 0;
	if (!isPv) { // Evaluate current position statically if current node is NOT a PV node
		if (tt_flag == FLAG_EXACT) { // Make use of previous TT score
			staticEval = tt_score;
		}
		else {
			Evaluate evaluate(s);
			staticEval = evaluate.getScore();
			if (tt_hit) {
				if (staticEval < tt_score && tt_flag == FLAG_LOWER || staticEval > tt_score && tt_flag == FLAG_UPPER) {
					staticEval = tt_score;
				}
			}
		}
	}
	
	// Reverse futility pruning
	if (!isPv && !isNull && !s.inCheck() && depth <= 2 && s.getNonPawnPieceCount(s.getOurColor())) {
		if (staticEval - 150 * depth >= beta) {
			return staticEval; /* beta */
		}
	}

	// Null move pruning
	// Make a null move and search to a reduced depth and check
	//   Current node is not PV node
	//   Current state not in check
	//   Depth is high enough
	//   Enough non-pawn pieces on board
	if (!isPv && !isNull && !s.inCheck() && depth > NULL_MOVE_DEPTH && s.getNonPawnPieceCount(s.getOurColor()) > NULL_MOVE_COUNT) {
		State n;
		std::memmove(&n, &s, sizeof s);
		n.makeNull();
		gi.history.push(std::make_pair(NULL_MOVE, n.getKey()));
		int nullScore = -search(n, si, gi, depth - 3, ply + 1, -(alpha + 1), -alpha, false, true, false);
		gi.history.pop();
		if (nullScore >= beta) {
			return beta;
		}
	}

	// Internal iterative deepening
	// In case no best move was found, perform a shallower search to determine which move to properly seach first
	if ((isPv || staticEval + 100 >= beta) && !isNull && !s.inCheck() && best_move == NULL_MOVE && depth >= 5) {
		search(s, si, gi, depth - 2, ply, alpha, beta, isPv, true, false);
		tt_entry = tt.probe(s.getKey());
		if (tt_entry.getKey() == s.getKey()) {
			best_move = tt_entry.getMove();
		}
	}

	MoveList mlist(s, best_move, &(gi.history), ply); // Generate movelist

	int a = alpha;
	int b = beta;
	int d;
	int score;
	int bestScore = NEG_INF;
	Move m;
	bool first = true;
	int count = 0;
	
	while ((m = mlist.getBestMove())) {
		d = depth - 1; // Early pruning
		if (!isPv && bestScore > -CHECKMATE_BOUND && !s.inCheck() && !s.givesCheck(m) && s.getNonPawnPieceCount(s.getOurColor())) {
			
			// Futility pruning
			//if (depth < 8 && !isPv && staticEval + 100 * d <= a) {
			//	continue;
			//}
			
			// Futility pruning
			if (depth == 1 && s.isQuiet(m) && !isPromotion(m) && staticEval + 300 < a) {
				continue;
			}

			// Late move reduction
			if (depth > 2 && mlist.size() > (isPv ? 5 : 3) && !s.inCheck() && !s.isCapture(m) && !isPromotion(m)) {
				d -= 1 + !isPv + (mlist.size() > 10);
				d = std::max(1, d);
			}

			// SEE-based pruning (prune if SEE too low)
			// Prune if see(move) < -(pawn * 2 ^ (depth - 1))
			if (depth < 3 && s.see(m) < -PAWN_WEIGHT * (1 << (depth - 1))) {
				continue;
			}
		}

		State c(s);
		c.makeMove(m); // Make move
		gi.history.push(std::make_pair(m, c.getKey())); // Add move to history
		count++;

		if (c.inCheck() && depth == 1) {
			d++;
		}

		// Search PV move
		if (first) {
			best_move = m;
			score = -search(c, si, gi, d, ply + 1, -b, -a, isPv, isNull, true);
			first = false;
		}     
		else {
			//if (d >= 1) {
			//	score = -search(c, si, gi, d - 1, ply + 1, -(a + 1), -a, false, isNull, false);
			//}
			score = -search(c, si, gi, d, ply + 1, -(a + 1), -a, false, isNull, false);
			if (score > a) {
				score = -search(c, si, gi, d, ply + 1, -b, -a, false, isNull, true);
			}
		}
		
		gi.history.pop(); // Pop move from gamelist

		if (!(si.nodes & 2047) && (si.quit || interrupt(si) || THREAD_STOP)) {
			return 0;
		}
		
		if (score > bestScore) {
			bestScore = score;
			best_move = m;
		}

		a = std::max(a, bestScore);

		// Alpha-Beta pruning
		if (a >= b) {
			a = b;
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

	if (bestScore == NEG_INF) { // Either completely lost or (hopefully) a draw
		return s.check() ? -CHECKMATE + ply : STALEMATE;
	}

	if (a > alpha && a < b && !si.quit) {
		gi.variation.pushToPv(best_move, s.getKey(), ply, a);
	}

	U64 flag = a >= b ? FLAG_LOWER
		: a > alpha ? FLAG_EXACT
			: FLAG_UPPER;
	
	tt.insert(best_move, flag, depth, value_to_tt(a, s.getFiftyMoveRule()), s.getKey());

	// assert(variation.getPvMove() != NULL_MOVE);
	// Fail-Hard alpha beta score
	return a;
}

// int search(State& s, SearchInfo& si, GlobalInfo& gi, int depth, int ply, int alpha, int beta, bool isPv, bool isNull, bool isRoot)
void parallel_search(State s, SearchInfo si, int depth, int alpha, int beta, int t) {
	auto& [value, valid] = results[t];
	auto& gi = global_info[t];
	valid = false;
	
	value = search(s, si, gi, depth, 0, alpha, beta, false, false, false);
	if (!THREAD_STOP) {
		THREAD_STOP = true;
		valid = true;
	}
}

// CPW: https://www.chessprogramming.org/Iterative_Deepening
Move iterative_deepening(State& s, SearchInfo& si, int NUM_THREADS) {
	for (int i = 0; i < NUM_THREADS; ++i) {
		global_info[i].nodes = 0;
		results[i].first = 0;
		results[i].second = false;
	}

	Move best_move = 0;
	int alpha = NEG_INF;
	int beta = POS_INF;
	int adelta = 0;
	int bdelta = 0;
	int results_index;
	bool failed = false;
	int score = 0;
	for (int d = 1; !si.quit && d <= MAX_PLY; ++d) {
		adelta = 0;
		bdelta = 0;
		do {
			failed = false;
			results_index = 0;
			THREAD_STOP = false;
			if (d > 4) {
				for (int i = 1; i < NUM_THREADS; ++i) {
					threads[i] = std::thread{parallel_search, s, si, d + (i & 1), alpha, beta, i};
				}
			
				results[0].first = search(s, si, global_info[0], d, 0, alpha, beta, true, false, true);
				THREAD_STOP = true;
			
				for (int i = 1; i < NUM_THREADS; ++i) {
					threads[i].join();
					if (results[i].second) {
						results_index = i;
					}
				}
			}
			else {
				results[0].first = search(s, si, global_info[0], d, 0, alpha, beta, true, false, true);
			}

			score = results[results_index].first;
			if (si.quit || interrupt(si)) {
				break;
			}
		
			global_info[results_index].variation.checkPv(s);

			std::cout << "info depth " << d;
			if (global_info[results_index].variation.isMate()) {
				int n = global_info[results_index].variation.getMateInN();
				std::cout << " score mate " << (score > 0 ? n : -n);
			}
			else {
				std::cout << " score cp " << score;
			}

			std::cout << " time " << si.clock.elapsed<std::chrono::milliseconds>() << " nodes " << si.nodes << " nps " << si.nodes / (si.clock.elapsed<std::chrono::milliseconds>() + 1) * 1000;
			global_info[results_index].variation.printPv();
			std::cout << std::endl;

			//if (si.nodes == si.prevNodes || si.nodes >= si.max_nodes || d >= si.depth) {
			//	break;
			//}
			if (si.clock.elapsed<std::chrono::milliseconds>() * 2 > si.moveTime) {
				break; // Insufficient time for next search iteration
			}

			si.prevNodes = si.nodes;
			si.nodes = 0;
			
			// Aspiration window calculations
			if (score <= alpha) {
				alpha = std::max(score - ASP_DELTA[adelta], NEG_INF);
				adelta++;
				failed = true;
			}
			else if (score >= beta) {
				beta = std::min(score + ASP_DELTA[bdelta], POS_INF);
				bdelta++;
				failed = true;
			}
		} while (failed);

		if (d > 1 && interrupt(si)) {
			break;
		}

		best_move = global_info[results_index].variation.getPvMove();

		if (d > 4) {
			alpha = score - 10;
			beta = score + 10;
		}
	} 
	
	return best_move;
}

Move search(State& s, SearchInfo& si, int NUM_THREADS) {
	//tt.clear();
	init_eval();
	return iterative_deepening(s, si, NUM_THREADS);
}

///