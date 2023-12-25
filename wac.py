import argparse
import subprocess
import sys

class TestPosition:
    def __init__(self, id="", fen="", best_move=""):
        self.id = id;
        self.fen = fen
        self.best_move = best_move;

def parse(s):
    return s.split("; ")

def main():
    print("[+] Moraband Win-at-Chess 1.0")

    parser = argparse.ArgumentParser(description="Test Moraband")
    parser.add_argument("engine", type=str, help="path to Moraband")
    parser.add_argument("test_positions_file", type=str, help="test positions file")
    parser.add_argument("move_time", type=int, help="move time in ms")
    parser.add_argument("-threads", type=int, help="search threads", default=1)

    args = parser.parse_args()

    engine = subprocess.Popen(args.engine, 
                    stderr=subprocess.PIPE,
                    stdout=subprocess.PIPE,
                    stdin=subprocess.PIPE,
                    universal_newlines=True)

    move_time = args.move_time

    test_positions = []
    total = 0
    with open(args.test_positions_file, "r") as test_positions_file:
        lines = test_positions_file.readlines()
        for line in lines:
            total += 1
            if line[-1] == "\n":
                line = line[:-1]
            info = parse(line) # Remove '\n'
            test_positions.append(TestPosition(total, info[0], info[1]))
        print(f"[+] Read {total} positions from {args.test_positions_file}")

    # Clear engine output 
    for i in range(9):
        engine_out = engine.stdout.readline().strip()
        engine.stdout.flush()

    # Set threads
    threads = args.threads
    engine.stdin.write(f"setoption name Threads value {threads}\n")
    
    print(f"[+] Searching each position to depth {depth} using {threads} threads")

    failed = 0
    for test_position in test_positions:
        engine.stdin.write("ucinewgame\n")
        engine.stdout.flush()
        engine.stdin.write("isready\n")
        engine.stdout.flush()

        engine.stdin.write(f"position fen {test_position.fen}\n")
        engine.stdin.write(f"go movetime {move_time}\n")
        engine.stdin.flush()

        engine_out = []

        while True:
            engine_out.append(engine.stdout.readline().strip())
            engine.stdout.flush()
            if "bestmove" in engine_out[-1]:
                break
        
        engine_best_move = engine_out[-1].split("bestmove ")[1]
        if engine_best_move not in test_position.best_move:
            print(f"[-] FAILED: id {test_position.id} fen {test_position.fen}")
            print(f"[-]   {engine_out[-2].split(' pv')[0]}")
            print(f"[-]   found {engine_best_move} expected {test_position.best_move}")
            failed += 1

    print(f"[+] Total tests:  {total}")
    print(f"[+] Failed tests: {failed} ({round(failed / total * 100, 2)}%)")

    engine.stdin.write("quit")
    engine.stdin.flush()

if __name__ == "__main__":
    main()