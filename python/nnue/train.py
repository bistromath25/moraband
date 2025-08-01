import argparse
import logging
import os
from multiprocessing import Pool, cpu_count

import chess
import numpy as np
import torch
import torch.nn as nn
import torch.optim as optim
from torch.utils.data import DataLoader, Dataset, TensorDataset

logging.basicConfig(
    level=logging.INFO,
    format="[%(asctime)s] %(levelname)s: %(message)s",
    datefmt="%H:%M:%S",
)

MAX_CP = 1_000
FEATURE_SIZE = 768
MAX_CHUNK_SIZE = 1_000_000
MAX_LOAD_WORKERS = 8


def get_default_device():
    if torch.backends.mps.is_available() and torch.backends.mps.is_built():
        return "mps"
    elif torch.cuda.is_available():
        return "cuda"
    else:
        return "cpu"


default_device = get_default_device()


# 2-layer feedforward network:
# - Input: 768-dimensional sparse binary feature vector
# - Hidden layer: 128 neurons, fully connected
# - Activation: Clipped ReLU
# - Output: Single scalar evaluation (no activation)
class NNUE(nn.Module):
    def __init__(self, hidden_dim=128):
        super().__init__()
        self.fc1 = nn.Linear(FEATURE_SIZE, hidden_dim)
        self.fc2 = nn.Linear(hidden_dim, 1)

    def forward(self, x):
        x = torch.clamp(self.fc1(x), min=0.0, max=1.0)
        return self.fc2(x)


def flip_board_for_opponent(board: chess.Board):
    flipped = board.mirror()
    flipped.turn = not board.turn
    return flipped


def encode_board(board: chess.Board, perspective=chess.WHITE):
    if board.turn != perspective:
        board = flip_board_for_opponent(board.copy())
    x = np.zeros(FEATURE_SIZE, dtype=np.float32)
    for square in chess.SQUARES:
        piece = board.piece_at(square)
        if piece:
            piece_index = (piece.piece_type - 1) + (
                0 if piece.color == chess.WHITE else 6
            )
            x[square * 12 + piece_index] = 1.0
    return x


def parse_line(line_tuple):
    i, line = line_tuple
    try:
        fen, score_str = line.strip().split(";")
        board = chess.Board(fen)
        x_tensor = torch.tensor(
            encode_board(board, perspective=board.turn), dtype=torch.float32
        )
        raw_eval = int(score_str)
        clamped = max(-MAX_CP, min(MAX_CP, raw_eval)) / MAX_CP
        y_tensor = torch.tensor([clamped], dtype=torch.float32)
        return x_tensor, y_tensor
    except Exception as e:
        logging.warning(f"Line {i} skipped: {e}")
        return None


def parse_lines_parallel(line_tuples, num_workers):
    workers = min(num_workers, MAX_LOAD_WORKERS)
    with Pool(processes=workers) as pool:
        results = pool.map(parse_line, line_tuples)
    return [res for res in results if res is not None]


def load_fen_dataset_in_chunks(filename, chunk_size=MAX_CHUNK_SIZE, num_workers=4):
    with open(filename, "r") as f:
        buffer = []
        for i, line in enumerate(f, 1):
            if line.strip():
                buffer.append((i, line))
            if i % chunk_size == 0:
                logging.info(f"Parsing chunk at line {i:,}...")
                yield parse_lines_parallel(buffer, num_workers)
                buffer.clear()
        if buffer:
            logging.info(f"Parsing final chunk ({len(buffer):,} lines)...")
            yield parse_lines_parallel(buffer, num_workers)


def save_nnue_weights(model, path):
    fc1_w = model.fc1.weight.detach().cpu().numpy()
    fc1_b = model.fc1.bias.detach().cpu().numpy()
    fc2_w = model.fc2.weight.detach().cpu().numpy()
    fc2_b = model.fc2.bias.detach().cpu().numpy()
    weights = np.concatenate(
        [fc1_w.T.flatten(), fc1_b.flatten(), fc2_w.flatten(), fc2_b.flatten()]
    ).astype(np.float32)
    weights.tofile(path)
    logging.info(f"Saved NNUE weights to {path} ({weights.size} floats)")


def train_nnue_chunked(
    fen_eval_path,
    model_save_path,
    batch_size=64,
    epochs=10,
    chunk_size=MAX_CHUNK_SIZE,
    learning_rate=1e-3,
    weight_decay=1e-6,
    load_workers=4,
    device=default_device,
):
    model = NNUE().to(device)
    criterion = nn.MSELoss()
    optimizer = optim.AdamW(
        model.parameters(), lr=learning_rate, weight_decay=weight_decay
    )

    logging.info("Training Configuration")
    logging.info(f"Training on device: {device}")
    logging.info(f"Epochs:             {epochs}")
    logging.info(f"Batch size:         {batch_size}")
    logging.info(f"Learning rate:      {learning_rate}")
    logging.info(f"Weight decay:       {weight_decay}")
    logging.info(f"Chunk size:         {chunk_size:,}")
    logging.info(f"Preprocessing workers: {min(load_workers, MAX_LOAD_WORKERS)}")

    for epoch in range(1, epochs + 1):
        logging.info(f"=== Epoch {epoch}/{epochs} ===")
        chunk_idx = 1
        for chunk_samples in load_fen_dataset_in_chunks(
            fen_eval_path, chunk_size, load_workers
        ):
            if not chunk_samples:
                logging.warning("Empty chunk - skipping.")
                continue
            logging.info(
                f"Training on chunk {chunk_idx} with {len(chunk_samples):,} positions"
            )

            x_batch, y_batch = zip(*chunk_samples)
            dataset = TensorDataset(torch.stack(x_batch), torch.stack(y_batch))
            dataloader = DataLoader(
                dataset,
                batch_size=batch_size,
                shuffle=True,
                num_workers=2,
                pin_memory=True,
                persistent_workers=True,
            )

            model.train()
            total_loss = 0.0
            for batch_inputs, batch_targets in dataloader:
                batch_inputs = batch_inputs.to(device)
                batch_targets = batch_targets.to(device)

                optimizer.zero_grad()
                with torch.autocast(
                    device_type=device,
                    dtype=torch.float16 if device != "cpu" else torch.bfloat16,
                ):
                    outputs = model(batch_inputs)
                    loss = criterion(outputs, batch_targets)

                loss.backward()
                optimizer.step()
                total_loss += loss.item() * batch_inputs.size(0)

            avg_loss = total_loss / len(dataset)
            logging.info(f"Chunk {chunk_idx} - Avg Loss: {avg_loss:.6f}")
            chunk_idx += 1

    logging.info("Training complete. Saving model...")
    save_nnue_weights(model, model_save_path)


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="Train NNUE model with chunked input")
    parser.add_argument(
        "--data", type=str, default="fens_evals.txt", help="Path to fens file"
    )
    parser.add_argument(
        "--model", type=str, default="nnue.bin", help="Where to save NNUE weights"
    )
    parser.add_argument("--batch_size", type=int, default=64)
    parser.add_argument("--epochs", type=int, default=10)
    parser.add_argument("--lr", type=float, default=1e-3)
    parser.add_argument("--decay", type=float, default=1e-6)
    parser.add_argument(
        "--device", type=str, choices=["cpu", "cuda", "mps"], default=default_device
    )
    parser.add_argument(
        "--load_workers",
        type=int,
        default=min(os.cpu_count() or 8, MAX_LOAD_WORKERS),
        help="Workers to preprocess data",
    )
    parser.add_argument(
        "--chunk_size",
        type=int,
        default=MAX_CHUNK_SIZE,
        help=f"Max number of samples per chunk ({MAX_CHUNK_SIZE})",
    )

    args = parser.parse_args()

    chunk_size = min(args.chunk_size, MAX_CHUNK_SIZE)

    train_nnue_chunked(
        fen_eval_path=args.data,
        model_save_path=args.model,
        batch_size=args.batch_size,
        epochs=args.epochs,
        chunk_size=chunk_size,
        learning_rate=args.lr,
        weight_decay=args.decay,
        load_workers=min(args.load_workers, MAX_LOAD_WORKERS),
        device=args.device,
    )
