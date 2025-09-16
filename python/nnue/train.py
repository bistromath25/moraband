import argparse
import hashlib
import logging
import math
import os
import subprocess
import time

import chess
import numpy as np
import torch
import torch.nn as nn
import torch.optim as optim
from torch.utils.data import DataLoader, IterableDataset

logging.basicConfig(
    level=logging.INFO,
    format="[%(asctime)s] %(levelname)s: %(message)s",
    datefmt="%H:%M:%S",
)

torch.set_float32_matmul_precision("high")

MAX_CP = 1000
INPUT_FEATURES = 768
HIDDEN_FEATURES = 256
DEVICE = torch.device("mps")


def format_time(seconds: float) -> str:
    minutes, sec = divmod(int(seconds), 60)
    return f"{minutes:02d}:{sec:02d}"


def encode_board(board: chess.Board):
    x = np.zeros(INPUT_FEATURES, dtype=np.float32)

    piece_map = board.piece_map()
    if piece_map:
        squares = np.fromiter(piece_map.keys(), dtype=np.int32, count=len(piece_map))
        pieces = np.fromiter(
            (p.piece_type for p in piece_map.values()),
            dtype=np.int32,
            count=len(piece_map),
        )
        colors = np.fromiter(
            (p.color for p in piece_map.values()), dtype=np.int32, count=len(piece_map)
        )

        piece_indices = (pieces - 1) + (1 - colors) * 6  # chess.WHITE == 1
        indices = squares * 12 + piece_indices
        x[indices] = 1.0

    return x


class NNUE(nn.Module):
    def __init__(self, hidden_dim=HIDDEN_FEATURES):
        super().__init__()
        self.fc1 = nn.Linear(INPUT_FEATURES, hidden_dim)
        self.fc2 = nn.Linear(hidden_dim, 1)

    def forward(self, x):
        x = torch.clamp(self.fc1(x), min=0.0, max=1.0)
        return self.fc2(x)


class FenEvalDataset(IterableDataset):
    def __init__(self, filename, max_cp=MAX_CP):
        super().__init__()
        self.filename = filename
        self.max_cp = max_cp

    def __iter__(self):
        with open(self.filename, "r") as file:
            for line in file:
                line = line.strip()
                if not line:
                    continue
                try:
                    fen, score_str = line.split(";")
                    board = chess.Board(fen)
                    x = encode_board(board)
                    raw_eval = int(score_str)
                    clamped = (
                        max(-self.max_cp, min(self.max_cp, raw_eval)) / self.max_cp
                    )
                    x_tensor = torch.from_numpy(x)
                    y_tensor = torch.tensor([clamped], dtype=torch.float32)
                    yield x_tensor, y_tensor
                except Exception as e:
                    logging.warning(f"Skipping line {line} due to error: {e}")
                    continue


def save_nnue_weights(model):
    fc1_w = model.fc1.weight.detach().cpu().numpy()
    fc1_b = model.fc1.bias.detach().cpu().numpy()
    fc2_w = model.fc2.weight.detach().cpu().numpy()
    fc2_b = model.fc2.bias.detach().cpu().numpy()
    # Transpose fc1_weights
    weights = np.concatenate(
        [fc1_w.T.flatten(), fc1_b.flatten(), fc2_w.flatten(), fc2_b.flatten()]
    ).astype(np.float32)
    digest = hashlib.sha256(weights.tobytes()).hexdigest()[:12]
    filename = f"nn-{digest}.nnue"
    weights.tofile(filename)
    logging.info(f"Saved NNUE weights to {filename} ({weights.size} floats)")


def train_nnue(
    fen_eval_path,
    batch_size=64,
    epochs=10,
    learning_rate=1e-3,
    weight_decay=1e-6,
    resume=False,
):
    start_epoch = 1
    model = NNUE().to(DEVICE)
    criterion = nn.MSELoss()
    optimizer = optim.AdamW(
        model.parameters(), lr=learning_rate, weight_decay=weight_decay
    )

    total_samples = int(
        subprocess.run(
            ["wc", "-l", fen_eval_path], capture_output=True, text=True, check=True
        ).stdout.split()[0]
    )
    total_batches = math.ceil(total_samples / batch_size)

    logging.info("Training Configuration")
    logging.info(f"Epochs:             {epochs}")
    logging.info(f"Batch size:         {batch_size}")
    logging.info(f"Learning rate:      {learning_rate}")
    logging.info(f"Weight decay:       {weight_decay}")
    logging.info(f"Total lines:        {total_samples}")
    logging.info(f"Total batches:      {total_batches}")

    if resume and os.path.exists("checkpoint.pth"):
        checkpoint = torch.load("checkpoint.pth", map_location=DEVICE)
        model.load_state_dict(checkpoint["model_state"])
        optimizer.load_state_dict(checkpoint["optimizer_state"])
        start_epoch = checkpoint["epoch"] + 1
        logging.info(f"Resuming from epoch {start_epoch}")

    dataset = FenEvalDataset(fen_eval_path)
    dataloader = DataLoader(dataset, batch_size=batch_size, num_workers=0)

    for epoch in range(start_epoch, epochs + 1):
        logging.info(f"=== Epoch {epoch}/{epochs} ===")
        total_loss = 0.0
        total_samples_so_far = 0
        model.train()
        start_time = time.time()

        for batch_idx, (batch_inputs, batch_targets) in enumerate(dataloader, 1):
            batch_inputs = batch_inputs.to(DEVICE)
            batch_targets = batch_targets.to(DEVICE)

            optimizer.zero_grad()
            outputs = model(batch_inputs)
            loss = criterion(outputs, batch_targets)
            loss.backward()
            optimizer.step()

            total_loss += loss.item() * batch_inputs.size(0)
            total_samples_so_far += batch_inputs.size(0)

            if batch_idx % 10000 == 0 or batch_idx == total_batches:
                elapsed = time.time() - start_time
                batches_done = batch_idx
                batches_left = total_batches - batches_done
                eta_seconds = (
                    elapsed / batches_done * batches_left if batches_done > 0 else 0
                )
                eta_str = format_time(eta_seconds)
                avg_loss = total_loss / total_samples_so_far
                logging.info(
                    f"Epoch {epoch} [Batch {batch_idx}/{total_batches}] - "
                    f"Batch Loss: {loss.item():.6f}, "
                    f"Avg Loss: {avg_loss:.6f}, "
                    f"ETA: {eta_str}"
                )

        avg_loss = (
            total_loss / total_samples_so_far
            if total_samples_so_far > 0
            else float("nan")
        )
        logging.info(f"Epoch {epoch} - Avg Loss: {avg_loss:.6f}")
        torch.save(
            {
                "epoch": epoch,
                "model_state": model.state_dict(),
                "optimizer_state": optimizer.state_dict(),
            },
            "checkpoint.pth",
        )
        logging.info(f"Checkpoint saved at epoch {epoch}")

    logging.info("Training complete. Saving model...")
    save_nnue_weights(model)


def main():
    if not torch.backends.mps.is_available():
        raise RuntimeError("MPS backend not available")

    parser = argparse.ArgumentParser(
        description="Train NNUE model using PyTorch Dataset on MPS"
    )
    parser.add_argument(
        "--data", type=str, default="fens_evals.txt", help="Path to fens file"
    )
    parser.add_argument("--batch-size", type=int, default=64)
    parser.add_argument("--epochs", type=int, default=10)
    parser.add_argument("--learning-rate", type=float, default=1e-3)
    parser.add_argument("--decay", type=float, default=1e-6)
    parser.add_argument(
        "--resume", action="store_true", help="Resume training from checkpoint"
    )
    args = parser.parse_args()

    train_nnue(
        fen_eval_path=args.data,
        batch_size=args.batch_size,
        epochs=args.epochs,
        learning_rate=args.learning_rate,
        weight_decay=args.decay,
        resume=args.resume,
    )


if __name__ == "__main__":
    main()
