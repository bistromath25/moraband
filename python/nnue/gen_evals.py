import argparse
import logging
import os
from multiprocessing import Pool, cpu_count
from pathlib import Path

import chess
import chess.engine

logging.basicConfig(
    level=logging.INFO,
    format="[%(asctime)s] %(levelname)s: %(message)s",
    datefmt="%H:%M:%S",
)

FEN_PATH = "fens.txt"
CACHE_PATH_BASE = "fens_evals"
STOCKFISH_PATH = "/opt/homebrew/bin/stockfish"
DEPTH = 6
NUM_WORKERS = cpu_count() or 4
MAX_FENS = 10_000_000


# Persistent Worker Engine
class EngineWorker:
    def __init__(self, stockfish_path: str, depth: int):
        self.engine = chess.engine.SimpleEngine.popen_uci(stockfish_path)
        self.depth = depth

    def evaluate(self, fen: str) -> int:
        try:
            board = chess.Board(fen)
            info = self.engine.analyse(board, chess.engine.Limit(depth=self.depth))
            score = info["score"].white().score(mate_score=100000)
            return score if score is not None else 0
        except Exception as e:
            logging.warning(f"Error evaluating FEN {fen}: {e}")
            return 0

    def close(self):
        self.engine.quit()


# Global worker instance
worker = None


def init_worker(stockfish_path: str, depth: int):
    global worker
    worker = EngineWorker(stockfish_path, depth)


def fen_worker(task):
    global worker
    idx, fen = task
    score = worker.evaluate(fen)
    return idx, fen, score


def clean_fen(fen: str) -> str:
    return fen.split("[")[0].strip()


def load_fens(path: str, max_fens: int = None):
    logging.info(f"Loading FENs from {path}")
    with open(path, "r") as f:
        lines = [clean_fen(line.strip()) for line in f if line.strip()]
    if max_fens:
        lines = lines[:max_fens]
    logging.info(f"Loaded {len(lines)} FENs")
    return lines


def save_fens(cache_path: str, results):
    with open(cache_path, "w") as f:
        for _, fen, score in results:
            f.write(f"{fen};{score}\n")
    logging.info(f"Results written to {cache_path}")


def generate_fen_evals_slice(
    fen_path: str,
    workers: int,
    engine_path: str,
    depth: int,
    start_idx: int = 0,
    limit: int = None,
    max_fens: int = None,
):
    fens = load_fens(fen_path, max_fens=max_fens or MAX_FENS)
    end_idx = min(start_idx + limit, len(fens)) if limit else len(fens)
    fens_slice = fens[start_idx:end_idx]
    total = len(fens_slice)
    cache_path = f"{CACHE_PATH_BASE}_{start_idx}_{end_idx}.txt"

    logging.info("Evaluation Configuration")
    logging.info(f"Engine Path:       {engine_path}")
    logging.info(f"Engine Depth:      {depth}")
    logging.info(f"Worker Processes:  {workers}")
    logging.info(f"FEN Start Index:   {start_idx}")
    logging.info(f"FEN Limit:         {limit}")
    logging.info(f"Total FENs:        {total}")
    logging.info(f"Cache File Path:   {cache_path}")

    args_list = [(i, fen) for i, fen in enumerate(fens_slice)]

    with Pool(
        processes=workers,
        initializer=init_worker,
        initargs=(engine_path, depth),
    ) as pool:
        results = pool.imap_unordered(fen_worker, args_list)
        result_buffer = [None] * total
        completed = 0

        for idx, fen, score in results:
            result_buffer[idx] = (idx, fen, score)
            completed += 1
            if completed % 10_000 == 0 or completed == total:
                logging.info(f"Evaluated {completed}/{total} positions")

    save_fens(cache_path, result_buffer)
    logging.info("All evaluations complete.")
    return result_buffer


def main():
    parser = argparse.ArgumentParser(description="Evaluate chess FENs using Stockfish")
    parser.add_argument("--fens", type=str, default=FEN_PATH)
    parser.add_argument("--start", type=int, default=0, help="Start index (inclusive)")
    parser.add_argument(
        "--limit", type=int, default=None, help="Number of FENs to process"
    )
    parser.add_argument(
        "--max-fens", type=int, default=MAX_FENS, help="Max FENs to load"
    )
    parser.add_argument(
        "--workers", type=int, default=NUM_WORKERS, help="Number of worker processes"
    )
    parser.add_argument("--engine-path", type=str, default=STOCKFISH_PATH)
    parser.add_argument("--depth", type=int, default=DEPTH, help="Analysis depth")
    args = parser.parse_args()

    generate_fen_evals_slice(
        fen_path=args.fens,
        workers=args.workers,
        engine_path=args.engine_path,
        depth=args.depth,
        start_idx=args.start,
        limit=args.limit,
        max_fens=args.max_fens,
    )


if __name__ == "__main__":
    main()
