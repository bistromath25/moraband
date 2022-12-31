#include "search.h"
#include "io.h"
#include <string>
#include <fstream>

// CPW: https://www.chessprogramming.org/Futility_Pruning

Variation variation;
History history;

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

int qsearch(State& s, SearchInfo& si, int ply, int alpha, int beta) {
	si.nodes++;
	assert(ply < Max_ply);

	if (history.isThreefoldRepetition(s) || s.insufficientMaterial() || s.getFiftyMoveRule() > 99) {
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
	MoveList mlist(s, NULL_MOVE, &history, ply, true);

	int bestScore = NEG_INF;
	int score;
	Move m;

	// Futility pruning: do not search subtrees which are unlikely to improve the alpha value score
	while (m = mlist.getBestMove()) {
		// Avoid pruning tactical positions
		if (!s.inCheck() && !s.givesCheck(m) && !s.isEnPassant(m) && !isPromotion(m)) {
			int fScore = qscore + 100 + PieceValue[s.onSquare(getDst(m))];
			if (fScore <= alpha) {
				bestScore = std::max(bestScore, fScore);
				continue;
			}
		}

		State c(s);
		c.make_t(m);

		history.push(std::make_pair(m, c.getKey()));
		score = -qsearch(c, si, ply + 1, -beta, -alpha);
		history.pop();
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

int scout_search(State& s, SearchInfo& si, int depth, int ply, int alpha, int beta, bool isPv, bool isNull, bool isRoot) {
	assert(depth >= 0);
	Move best_move = NULL_MOVE;

	if (si.quit || (si.nodes % 3000 == 0 && interrupt(si))) {
		return 0;
	}

	si.nodes++;

	if (depth == 0) {
		return qsearch(s, si, ply, alpha, beta);
	}
	if (!isRoot && (history.isThreefoldRepetition(s) || s.insufficientMaterial() || s.getFiftyMoveRule() > 99)) {
		return DRAW;
	}

	const TableEntry* table_entry = ttable.probe(s.getKey());
	
	if (table_entry->key == s.getKey() && table_entry->depth >= depth) {
		if (table_entry->type == pv) { // PV node
			return table_entry->score;
		}
		else if (table_entry->type == cut) { // Cut node
			if (table_entry->score >= beta) {
				return beta;
			}
		}
		else {
			if (table_entry->score <= alpha) {
				return alpha;
			}
		}
		best_move = table_entry->best;
	}

	// Check PV variation
	if (variation.getPvKey(ply) == s.getKey()) {
		best_move = variation.getPvMove(ply);
	}

	int staticEval = 0;
	if (!isPv) { // Evaluate current position statically if current node is NOT a PV node
		Evaluate evaluate(s);
		staticEval = evaluate.getScore();
	}
	
	// Reverse futility pruning
	if (!isPv && !isNull && !s.inCheck() && depth <= 2 && s.getNonPawnPieceCount(s.getOurColor())) {
		if (staticEval - 100 * depth >= beta) {
			return beta;
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
		history.push(std::make_pair(NULL_MOVE, n.getKey()));
		int nullScore = -scout_search(n, si, depth - 3, ply + 1, -(alpha + 1), -alpha, false, true, false);
		history.pop();
		if (nullScore >= beta) {
			return beta;
		}
	}

	// Internal iterative deepening
	// In case no best move was found, perform a shallower search to determine which move to properly seach first
	if (isPv && !isNull && !s.inCheck() && best_move == NULL_MOVE && depth > 5) {
		// Using the Stockfish depth calculation
		int d = 3 * depth / 4 - 2;
		scout_search(s, si, d, ply, alpha, beta, isPv, true, false);
		table_entry = ttable.probe(s.getKey());
		if (table_entry->key == s.getKey()) {
			best_move = table_entry->best;
		}
	}

	MoveList mlist(s, best_move, &history, ply); // Generate movelist

	int a = alpha;
	int b = beta;
	int d;
	int score;
	int bestScore = NEG_INF;
	Move m;
	bool first = true;
	int count = 0;
	
	while (m = mlist.getBestMove()) {
		d = depth - 1; // Early pruning
		if (!isPv && bestScore > -CHECKMATE_BOUND && !s.inCheck() && !s.givesCheck(m) && s.getNonPawnPieceCount(s.getOurColor())) {
			// Futility pruning
			if (depth == 1 && s.isQuiet(m) && !isPromotion(m) && staticEval + 300 < a) {
				continue;
			}
			// SEE-based pruning (prune if SEE too low)
			// Prune if see(move) < -(pawn * 2 ^ (depth - 1))
			if (depth < 3 && s.see(m) < -PAWN_WEIGHT * (1 << (depth - 1)))
				continue;
		}

		State c(s);
		c.make_t(m); // Make move
		history.push(std::make_pair(m, c.getKey())); // Add move to history
		count++;

		if (c.inCheck() && depth == 1) {
			d++;
		}

		// Search first move
		if (first) {
			best_move = m;
			score = -scout_search(c, si, d, ply + 1, -b, -a, isPv, isNull, false);
			first = false;
		}     
		else {
			// Late move reduction
			// Seach with reduced depth under conditions described below
			// CPW: https://www.chessprogramming.org/Late_Move_Reductions
			if (count > LMR_COUNT && depth > LMR_DEPTH && !isPv && !s.inCheck() && !c.inCheck() && !s.isCapture(m) && !isPromotion(m)) {
				score = -scout_search(c, si, d - 1, ply + 1, -(a + 1), -a, false, isNull, false);
			}
			else {
				score = a + 1;
			}
			if (score > a) {
				score = -scout_search(c, si, d, ply + 1, -(a + 1), -a, false, isNull, false);
			}
			// If an alpha improvement resulted in a fail-high, search again with a full window
			if (a < score && b > score) {
				score = -scout_search(c, si, d, ply + 1, -b, -a, true, isNull, false);
			}
		}
		
		history.pop(); // Pop move from gamelist

		if (score > bestScore) {
			best_move = m;
			bestScore = score;
		}

		a = std::max(a, bestScore);

		// Alpha-Beta pruning
		if (a >= b) {
			a = b;
			if (s.isQuiet(m)) {
				history.update(m, depth, ply, true);
			}
			break;
		}
		else {
			if (s.isQuiet(m)) {
				history.update(m, depth, ply, false);
			}
		}
	}

	if (bestScore == NEG_INF) { // Either completely lost or (hopefully) a draw
		return s.check() ? -CHECKMATE + ply : STALEMATE;
	}
	if (a > alpha && a < b && !si.quit) {
		variation.pushToPv(best_move, s.getKey(), ply, a);
	}

	ttable.store(s.getKey(), best_move, a <= alpha ? all : a >= b ? cut : pv, depth, a);

	assert(variation.getPvMove() == NULL_MOVE); // Sometimes the PV is empty
	// Fail-Hard alpha beta score
	return a;
}

// CPW: https://www.chessprogramming.org/Iterative_Deepening
void iterative_deepening(State& s, SearchInfo& si) {
	int score;
	for (int d = 1; !si.quit; ++d) {
		if (d == si.depth) {
			break;
		}

		score = scout_search(s, si, d, 0, NEG_INF, POS_INF, true, false, true);

		if (variation.getPvMove() == NULL_MOVE) {
#ifdef DEBUG_ACCEPT_NULL_MOVE
			std::cout << "bestmove 0000" << std::endl; // Something must be wrong
#else
			//MoveList mlist(s); // Try to make a (random?) move
			//std::cout << "bestmove " << toString(mlist.getBestMove()) << " MAYBE ILLEGAL" << std::endl;
			ttable.clear();
			std::cout << "CLEARED TRANSPOSITION TABLE" << std::endl;
			//engine_log << "CLEARED TRANSPOSITION TABLE" << std::endl;
			//engine_output += "CLEARED TRANSPOSITION TABLE\n";
			score = scout_search(s, si, d, 0, NEG_INF, POS_INF, true, false, true);
#endif
		}
		
		if (si.quit) {
			break;
		}
		
		variation.checkPv(s);

		// Print search info
		std::cout << "info depth " << d;
		//engine_log << "info depth " << d;
		//engine_output += "info depth " + std::to_string(d);
		if (variation.isMate()) {
			int n = variation.getMateInN();
			std::cout << " score mate " << (score > 0 ? n : -n);
			//engine_log << " score mate " << (score > 0 ? n : -n);
			//engine_output += " score mate " + std::to_string(score > 0 ? n : -n);
		}
		else {
			std::cout << " score cp " << score;
			//engine_log << " score cp " << score;
			//engine_output += " score cp " + std::to_string(score);
		}

		std::cout << " time " << si.clock.elapsed<std::chrono::milliseconds>() << " nodes " << si.nodes << " nps " << si.nodes / (si.clock.elapsed<std::chrono::milliseconds>() + 1) * 1000;
		//engine_log << " time " << si.clock.elapsed<std::chrono::milliseconds>() << " nodes " << si.nodes << " nps " << si.nodes / (si.clock.elapsed<std::chrono::milliseconds>() + 1) * 1000;
		//engine_output += " time " + std::to_string(si.clock.elapsed<std::chrono::milliseconds>()) + " nodes " + std::to_string(si.nodes) + " nps " + std::to_string(si.nodes / (si.clock.elapsed<std::chrono::milliseconds>() + 1) * 1000);
		variation.printPv();
		std::cout << std::endl;
		//engine_log << std::endl;

		if (si.nodes == si.prevNodes) {
			break;
		}
		if (si.clock.elapsed<std::chrono::milliseconds>() * 2 > si.moveTime) {
			break; // Insufficient time for next search iteration
		}

		si.prevNodes = si.nodes;
		si.nodes = 0;
	}
	
	std::cout << "bestmove " << toString(variation.getPvMove()) << std::endl;
	//engine_log << "bestmove " << toString(variation.getPvMove()) << std::endl;
	//engine_output += "bestmove " + toString(variation.getPvMove()) + "\n";
}

void setup_search(State& s, SearchInfo& si) {
	ttable.clear();
	variation.clearPv();
	init_eval();
	history.clear();
	iterative_deepening(s, si);
}

///