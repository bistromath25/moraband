import argparse
import datetime
import subprocess
import threading
import time
from concurrent.futures import ThreadPoolExecutor, as_completed

from tqdm import tqdm


class TestPosition:
    def __init__(
        self, id="", fen="", best_moves=None, is_best_move=True, is_chess960=False
    ):
        self.id = id
        self.fen = fen
        self.best_moves = best_moves or []
        self.is_best_move = is_best_move
        self.is_chess960 = is_chess960


def parse(line):
    parts = line.split(";")
    fen_and_cmd = parts[0].strip()
    id_part = parts[1].strip() if len(parts) > 1 else ""

    if " bm " in fen_and_cmd:
        fen, moves = fen_and_cmd.split(" bm ")
        is_best = True
    elif " am " in fen_and_cmd:
        fen, moves = fen_and_cmd.split(" am ")
        is_best = False

    fen = fen.strip()
    uci_moves = moves.split()

    test_id = ""
    if id_part.startswith("id "):
        start = id_part.find('"')
        end = id_part.rfind('"')
        if start != -1 and end != -1 and end > start:
            test_id = id_part[start + 1 : end]

    if test_id.startswith("chess960."):
        is_chess960 = True
    else:
        is_chess960 = False

    return TestPosition(test_id, fen, uci_moves, is_best, is_chess960)


class EngineWorker:
    def __init__(self, engine_path):
        self.engine_path = engine_path

        self.proc = subprocess.Popen(
            self.engine_path,
            stderr=subprocess.PIPE,
            stdout=subprocess.PIPE,
            stdin=subprocess.PIPE,
            universal_newlines=True,
            bufsize=1,
        )

        for _ in range(9):
            self.proc.stdout.readline()

        self.lock = threading.Lock()

    def run_position(self, test_position):
        with self.lock:
            self.proc.stdin.write("ucinewgame\n")
            self.proc.stdin.flush()

            self.proc.stdin.write("isready\n")
            self.proc.stdin.flush()
            self.proc.stdout.readline()

            if test_position.is_chess960:
                self.proc.stdin.write("setoption name UCI_Chess960 value true\n")
                self.proc.stdin.flush()

            self.proc.stdin.write(f"position fen {test_position.fen}\n")
            self.proc.stdin.flush()

            self.proc.stdin.write("moves\n")
            self.proc.stdin.flush()

            line = self.proc.stdout.readline().strip()
            num_moves = int(line.split()[0])
            engine_moves = [
                self.proc.stdout.readline().strip() for _ in range(num_moves)
            ]

            success = True
            if test_position.is_best_move:
                # All bm moves must be present
                for mv in test_position.best_moves:
                    if mv not in engine_moves:
                        success = False
                        break
            else:
                # All am moves must be absent
                for mv in test_position.best_moves:
                    if mv in engine_moves:
                        success = False
                        break

            return (test_position, engine_moves, success)

    def quit(self):
        self.proc.stdin.write("quit\n")
        self.proc.stdin.flush()
        self.proc.wait()


def main():
    parser = argparse.ArgumentParser(description="Test Moraband")
    parser.add_argument("engine", type=str, help="path to Moraband")
    parser.add_argument("test_positions_file", type=str, help="test positions file")
    parser.add_argument(
        "--workers", type=int, help="parallel engine processes", default=4
    )
    parser.add_argument(
        "--verbose", action="store_true", help="Print detailed failure information"
    )
    args = parser.parse_args()

    with open(args.test_positions_file, "r") as f:
        test_positions = [parse(line.strip()) for line in f if line.strip()]

    total = len(test_positions)

    print("Moraband Win-at-Chess 1.1")
    print(f"Positions: {total}")
    print(f"Workers:   {args.workers}")

    engines = [EngineWorker(args.engine) for _ in range(args.workers)]

    passed = 0
    failed = 0
    start_time = time.perf_counter()

    with ThreadPoolExecutor(max_workers=args.workers) as executor:
        futures = []
        for i, pos in enumerate(test_positions):
            worker = engines[i % len(engines)]
            futures.append(executor.submit(worker.run_position, pos))

        for future in tqdm(
            as_completed(futures),
            total=len(futures),
            desc="Running tests",
            unit="test",
            bar_format="{desc}: {n}/{total}",
        ):
            test_position, engine_moves, success = future.result(timeout=5.0)
            if success:
                passed += 1
            else:
                failed += 1
                if args.verbose:
                    print(f"FAILED: id {test_position.id} fen {test_position.fen}")
                    if test_position.is_best_move:
                        print(f"Include: {test_position.best_moves}")
                    else:
                        print(f"Exclude: {test_position.best_moves}")
                    print(f"Found: {engine_moves}")

    elapsed = time.perf_counter() - start_time

    for e in engines:
        e.quit()

    print(f"Passed: {passed} ({passed / total * 100:.2f}%)")
    print(f"Failed: {failed} ({failed / total * 100:.2f}%)")
    minutes, seconds = divmod(int(elapsed), 60)
    print(f"Time:   {minutes:02d}:{seconds:02d}")


if __name__ == "__main__":
    main()
