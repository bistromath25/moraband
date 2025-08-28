import argparse
import subprocess
import threading
import time
from concurrent.futures import ThreadPoolExecutor, as_completed

DEFAULT_MOVETIME = 5000


class TestPosition:
    def __init__(self, id="", fen="", best_moves=None):
        self.id = id
        self.fen = fen
        self.best_moves = best_moves or []


def parse(line):
    parts = line.split(";")
    fen_and_bm = parts[0].strip()
    id_part = parts[1].strip() if len(parts) > 1 else ""

    tokens = fen_and_bm.split(" bm ")
    fen = tokens[0].strip()
    uci_moves = tokens[1].split()

    test_id = ""
    if id_part.startswith("id "):
        start = id_part.find('"')
        end = id_part.rfind('"')
        if start != -1 and end != -1 and end > start:
            test_id = id_part[start + 1 : end]

    return TestPosition(test_id, fen, uci_moves)


class EngineWorker:
    def __init__(self, engine_path, move_time=DEFAULT_MOVETIME, depth=None):
        self.engine_path = engine_path
        self.move_time = move_time
        self.depth = depth

        self.proc = subprocess.Popen(
            self.engine_path,
            stderr=subprocess.PIPE,
            stdout=subprocess.PIPE,
            stdin=subprocess.PIPE,
            universal_newlines=True,
            bufsize=1,
        )

        for _ in range(5):
            self.proc.stdout.readline()

        self.lock = threading.Lock()

    def run_position(self, test_position):
        with self.lock:
            self.proc.stdin.write("ucinewgame\n")
            self.proc.stdin.flush()
            self.proc.stdin.write("isready\n")
            self.proc.stdin.flush()
            self.proc.stdin.write(f"position fen {test_position.fen}\n")

            go_cmd = f"go movetime {self.move_time}"
            if self.depth is not None:
                go_cmd += f" depth {self.depth}"

            self.proc.stdin.write(go_cmd + "\n")
            self.proc.stdin.flush()

            engine_out = []
            while True:
                line = self.proc.stdout.readline().strip()
                if line:
                    engine_out.append(line)
                if "bestmove" in line:
                    break

            engine_best_move = engine_out[-1].split("bestmove ")[1]
            success = engine_best_move in test_position.best_moves
            return (test_position, engine_best_move, success, engine_out)

    def quit(self):
        self.proc.stdin.write("quit\n")
        self.proc.stdin.flush()
        self.proc.wait()


def main():
    start_time = time.perf_counter()

    parser = argparse.ArgumentParser(description="Test Moraband")
    parser.add_argument("engine", type=str, help="path to Moraband")
    parser.add_argument("test_positions_file", type=str, help="test positions file")
    parser.add_argument(
        "--move_time", type=int, help="move time in ms", default=DEFAULT_MOVETIME
    )
    parser.add_argument("--depth", type=int, help="search depth", default=None)
    parser.add_argument(
        "--workers", type=int, help="parallel engine processes", default=4
    )
    args = parser.parse_args()

    with open(args.test_positions_file, "r") as f:
        test_positions = [parse(line.strip()) for line in f if line.strip()]

    total = len(test_positions)

    print("Moraband Win-at-Chess 1.1")
    print(f"Positions: {total}")
    print(f"Workers:   {args.workers}")
    if args.move_time is not None:
        print(f"Movetime:  {args.move_time}")
    if args.depth is not None:
        print(f"Depth:     {args.depth}")

    engines = [
        EngineWorker(args.engine, args.move_time, args.depth)
        for _ in range(args.workers)
    ]

    passed = 0
    failed = 0
    with ThreadPoolExecutor(max_workers=args.workers) as executor:
        futures = []
        for i, pos in enumerate(test_positions):
            worker = engines[i % len(engines)]
            futures.append(executor.submit(worker.run_position, pos))

        for future in as_completed(futures):
            test_position, engine_best_move, success, engine_out = future.result()
            if success:
                passed += 1
            else:
                failed += 1
                print(f"FAILED: id {test_position.id} fen {test_position.fen}")
                if len(engine_out) > 1:
                    print(f"\t{engine_out[-2].split(' pv')[0]}")
                print(f"\tfound {engine_best_move} expected {test_position.best_moves}")

    for e in engines:
        e.quit()

    elapsed = time.perf_counter() - start_time

    print(f"Total tests:  {total}")
    print(f"Passed tests: {passed} ({passed / total * 100:.2f}%)")
    print(f"Failed tests: {failed} ({failed / total * 100:.2f}%)")
    print(f"Elapsed time: {elapsed:.2f} s")


if __name__ == "__main__":
    main()
