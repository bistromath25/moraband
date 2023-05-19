#include "search.h"
#include "io.h"
#include <string>
#include <fstream>
#include <thread>

// Check if search should be stopped
bool stop_search(SearchInfo& si) {
	// Not enough time left for search
	if (si.clock.elapsed<std::chrono::milliseconds>() >= si.moveTime) {
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
	return false;
}

int qsearch(State& s, SearchInfo& si, GlobalInfo& gi, int ply, int alpha, int beta) {
	si.nodes++;
	assert(ply <= MAX_PLY);

	if (gi.history.isThreefoldRepetition(s) || s.insufficientMaterial() || s.getFiftyMoveRule() > 99) {
		return DRAW; // Game must be a draw, return
	}

	if (!(si.nodes & 2047) && (si.quit || stop_search(si) || THREAD_STOP)) {
		return 0;
	}

	Evaluate evaluate(s);
	int staticEval = evaluate.getScore();
	if (staticEval >= beta) {
		return beta;
	}
	if (staticEval > alpha) {
		alpha = staticEval;
	}
	
	int bestScore = NEG_INF;
	int score;
	Move m = NULL_MOVE;

	// Generate moves and create the movelist.
	MoveList mlist(s, NULL_MOVE, &gi.history, ply, true);

	// Futility pruning: do not search subtrees which are unlikely to improve the alpha value score
	while ((m = mlist.getBestMove())) {
		// Avoid pruning tactical positions
		if (!s.inCheck() && !s.givesCheck(m) && !s.isEnPassant(m) && !isPromotion(m)) {
			int fScore = staticEval + 100 + PieceValue[s.onSquare(getDst(m))];
			if (fScore <= alpha) {
				bestScore = std::max(bestScore, fScore);
				continue;
			}
		}

		State c(s);
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
		}
		if (alpha >= beta) {
			return beta;
		}
	}

	if (bestScore == NEG_INF && s.check()) {
		return -CHECKMATE + ply;
	}
	
	return alpha;
}

int search(State& s, SearchInfo& si, GlobalInfo& gi, int depth, int ply, int alpha, int beta, bool isPv, bool isNull, bool isRoot, bool isPruning) {
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

	// Try getting the pv move
	//if (gi.variation.getPvKey(ply) == s.getKey()) {
	//	best_move = gi.variation.getPvMove(ply);
	//}
	
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

	// Pruning
	if (isPruning && !isPv && !s.inCheck() && s.getNonPawnPieceCount(s.getOurColor()) && beta > -CHECKMATE_BOUND) {
		// Reverse futility pruning
		if (depth < REVERSE_FUTILITY_DEPTH && staticEval - REVERSE_FUTILITY_MARGIN * depth >= beta) {
			return staticEval;
		}
		
		// Razoring
		if (depth <= RAZORING_DEPTH && staticEval <= alpha - RAZORING_MARGIN[depth]) {
			int rscore = qsearch(s, si, gi, 0, alpha - RAZORING_MARGIN[depth], alpha - RAZORING_MARGIN[depth] + 1);
			if (rscore <= alpha - RAZORING_MARGIN[depth]) {
				return rscore;
			}
		}

		// Null move pruning
		// Make a null move and search to a reduced depth
		if (depth > NULL_MOVE_DEPTH && staticEval + NULL_MOVE_MARGIN >= beta) {
			State n;
			std::memmove(&n, &s, sizeof s);
			n.makeNull();
			gi.history.push(std::make_pair(NULL_MOVE, n.getKey()));
			int nullScore = -search(n, si, gi, depth - 3, ply + 1, -beta, -(beta - 1), false, true, false, false);
			gi.history.pop();
			if (nullScore >= beta) {
				if (nullScore >= CHECKMATE_BOUND) {
					nullScore = beta;
				}
				return nullScore;
			}
		}
	}

	// Internal iterative deepening
	// In case no best move was found, perform a shallower search to determine which move to properly seach first
	if (!tt_move && !s.inCheck() && (isPv || staticEval + 100 >= beta) && depth >= 5) {
		search(s, si, gi, depth - 2, ply, alpha, beta, isPv, true, false, false);
		tt_entry = tt.probe(s.getKey());
		tt_move = tt_entry.getMove();
	}

	MoveList mlist(s, tt_move, &(gi.history), ply);

	int score = NEG_INF;
	int bestScore = NEG_INF;
	Move best_move = tt_move;

	int legalMoves = 0;
	int a = alpha;
	int b = beta;
	Move m;
	bool first = true;
	int d;
	while ((m = mlist.getBestMove())) {
		d = depth - 1; // Early pruning
		legalMoves++;

		if (!isPv && bestScore > -CHECKMATE_BOUND && !s.inCheck() && !s.givesCheck(m) && s.getNonPawnPieceCount(s.getOurColor())) {
			// Futility pruning
			if (depth == 1 && s.isQuiet(m) && !isPromotion(m) && staticEval + 300 < a) {
				continue;
			}
			// Late move reduction
			if (depth > 2 && legalMoves > (isPv ? 5 : 3) && !s.inCheck()) {
				d -= 1 + !isPv + (mlist.size() > 10);
				d = std::max(1, d);
			}
			// SEE-based pruning (prune if SEE too low)
			// Prune if see(move) < -(pawn * 2 ^ (depth - 1))
			if (depth <= 8 && s.see(m) < -PAWN_WEIGHT_MG * (1 << (depth - 1))) { // m != tt_move
				continue;
			}
		}

		State c(s);
		c.makeMove(m); // Make move on new position
		gi.history.push(std::make_pair(m, c.getKey())); // Add move to history

		if (c.inCheck() && depth == 1) {
			d++;
		}

		// Search PV move
		if (first) {
			//best_move = m;
			score = -search(c, si, gi, d, ply + 1, -b, -a, isPv, isNull, true, false);
			first = false;
		}     
		else {
			score = -search(c, si, gi, d, ply + 1, -(a + 1), -a, false, isNull, false, true);
			if (score > a) {
				score = -search(c, si, gi, std::max(d, depth - 1), ply + 1, -(a + 1), -a, false, isNull, false, true);
			}
			if (a < score && score < b) {
				score = -search(c, si, gi, d, ply + 1, -b, -a, true, isNull, false, true);
			}
		}
		
		gi.history.pop(); // Pop move from history

		if (!(si.nodes & 2047) && (si.quit || stop_search(si) || THREAD_STOP)) {
			return 0;
		}
		
		if (score > bestScore) {
			bestScore = score;
			best_move = m;
			gi.variation.pushToPv(best_move, s.getKey(), ply, bestScore);
			a = std::max(a, bestScore);
		}
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

	U64 flag = a >= b ? FLAG_LOWER
		: a > alpha ? FLAG_EXACT
			: FLAG_UPPER;
	
	assert(best_move != NULL_MOVE);
	tt.insert(best_move, flag, depth, value_to_tt(a, ply), s.getKey());

	//assert(variation.getPvMove() != NULL_MOVE);
	return a;
}

int search_root(State& s, SearchInfo& si, GlobalInfo& gi, int depth, int ply, int alpha, int beta) {
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

	MoveList mlist(s, tt_move, &(gi.history), ply);
	
	int score = NEG_INF;
	int bestScore = NEG_INF;
	Move best_move = NULL_MOVE;

	int legalMoves = 0;
	int a = alpha;
	int b = beta;
	Move m;
	bool first = true;
	int d;
	while ((m = mlist.getBestMove())) {
		d = depth - 1;
		legalMoves++;

		State c(s);
		c.makeMove(m);
		gi.history.push(std::make_pair(m, c.getKey()));

		if (c.inCheck() && depth == 1) {
			d++;
		}

		if (first) {
			//best_move = m;
			score = -search(c, si, gi, d, ply + 1, -b, -a, true, false, true, false);
			first = false;
		}     
		else {
			score = -search(c, si, gi, d, ply + 1, -(a + 1), -a, false, false, false, true);	
			if (score > a) {
				score = -search(c, si, gi, std::max(d, depth - 1), ply + 1, -(a + 1), -a, false, false, false, true);
			}
			if (a < score && score < b) {
				score = -search(c, si, gi, d, ply + 1, -b, -a, true, true, false, true);
			}
		}
		
		gi.history.pop();

		if (!(si.nodes & 2047) && (si.quit || stop_search(si) || THREAD_STOP)) {
			return 0;
		}
		
		if (score > bestScore) {
			bestScore = score;
			best_move = m;
			gi.variation.pushToPv(best_move, s.getKey(), ply, bestScore);
			a = std::max(a, bestScore);
		}
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

	if (bestScore == NEG_INF) {
		return s.check() ? -CHECKMATE + ply : STALEMATE;
	}

	U64 flag = a >= b ? FLAG_LOWER
		: a > alpha ? FLAG_EXACT
			: FLAG_UPPER;

	assert(best_move != NULL_MOVE);
	tt.insert(best_move, flag, depth, value_to_tt(a, ply), s.getKey());

	return a;
}

void parallel_search(State s, SearchInfo si, int depth, int alpha, int beta, int t) {
	auto& [value, valid] = results[t];
	auto& gi = global_info[t];
	valid = false;
	//value = search(s, si, gi, depth, 0, alpha, beta, false, false, false, true);
	value = search_root(s, si, gi, depth, 0, alpha, beta);
	if (!THREAD_STOP) {
		THREAD_STOP = true;
		valid = true;
	}
}

// CPW: https://www.chessprogramming.org/Iterative_Deepening
Move iterative_deepening(State& s, SearchInfo& si) {
	Move best_move = NULL_MOVE;
	//int alpha = NEG_INF;
	//int beta = POS_INF;
	int results_index = 0;
	int score = 0;
	for (int d = 1; !si.quit && d < MAX_PLY; ++d) {
		results_index = 0;
		THREAD_STOP = false;
		if (d > 4) {
			for (int i = 1; i < NUM_THREADS; ++i) {
				threads[i] = std::thread{parallel_search, s, si, d + (i & 1), NEG_INF, POS_INF, i};
			}
		
			//results[0].first = search(s, si, global_info[0], d, 0, NEG_INF, POS_INF, true, false, true, false);
			results[0].first = search_root(s, si, global_info[0], d, 0, NEG_INF, POS_INF);
			THREAD_STOP = true;
		
			for (int i = 1; i < NUM_THREADS; ++i) {
				threads[i].join(); // join all threads at end of search
				//if (results[i].second) {
				//	results_index = i;
				//}
			}
		}
		else {
			//results[0].first = search(s, si, global_info[0], d, 0, NEG_INF, POS_INF, true, false, true, false);
			results[0].first = search_root(s, si, global_info[0], d, 0, NEG_INF, POS_INF);
		}

		score = results[results_index].first;
		global_info[results_index].variation.checkPv(s);
		if (si.quit || stop_search(si)) {
			break;
		}

		if (si.nodes == si.prevNodes) {
			break;
		}
		if (si.clock.elapsed<std::chrono::milliseconds>() * 2 > si.moveTime) {
			break; // Insufficient time for next search iteration
		}

		si.prevNodes = si.nodes;
		si.nodes = 0;

		//score = results[results_index].first;
		//global_info[results_index].variation.checkPv(s);
		D(std::cout << "alpha beta score: " << alpha << ' ' << beta << ' ' << score << std::endl;);
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

		if (si.clock.elapsed<std::chrono::milliseconds>() * 2 > si.moveTime) {
			break; // Insufficient time for next search iteration
		}
		if (d > 1 && stop_search(si)) {
			break;
		}
		if ((si.depth && d >= si.depth) || (si.max_nodes && si.totalNodes >= si.max_nodes)) {
			break;
		}
	}

	return best_move;
}

Move search(State& s, SearchInfo& si) {
	//tt.clear();
	for (int i = 0; i < NUM_THREADS; ++i) {
		global_info[i].variation.clearPv();
		//global_info[i].history.clear();
		global_info[i].nodes = 0;
		results[i].first = 0;
		results[i].second = false;
	}
	return iterative_deepening(s, si);
}

///