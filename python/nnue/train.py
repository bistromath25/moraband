import argparse
import logging
import os

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

MAX_CP = 1_000
FEATURE_SIZE = 768
DEVICE = torch.device("mps")


def encode_board(board: chess.Board, perspective=chess.WHITE):
    if board.turn != perspective:
        board = board.mirror()
        board.turn = perspective
    x = np.zeros(FEATURE_SIZE, dtype=np.float32)
    for square in chess.SQUARES:
        piece = board.piece_at(square)
        if piece:
            piece_index = (piece.piece_type - 1) + (
                0 if piece.color == chess.WHITE else 6
            )
            x[square * 12 + piece_index] = 1.0
    return x


class NNUE(nn.Module):
    def __init__(self, hidden_dim=128):
        super().__init__()
        self.fc1 = nn.Linear(FEATURE_SIZE, hidden_dim)
        self.fc2 = nn.Linear(hidden_dim, 1)

    def forward(self, x):
        x = torch.clamp(self.fc1(x), min=0.0, max=1.0)
        return self.fc2(x)


class FenEvalDataset(IterableDataset):
    def __init__(self, filename, max_cp=MAX_CP, perspective=chess.WHITE):
        super().__init__()
        self.filename = filename
        self.max_cp = max_cp
        self.perspective = perspective

    def __iter__(self):
        with open(self.filename, "r") as file:
            for line in file:
                line = line.strip()
                if not line:
                    continue
                try:
                    fen, score_str = line.split(";")
                    board = chess.Board(fen)
                    x = encode_board(board, perspective=self.perspective)
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


def save_nnue_weights(model, path):
    fc1_w = model.fc1.weight.detach().cpu().numpy()
    fc1_b = model.fc1.bias.detach().cpu().numpy()
    fc2_w = model.fc2.weight.detach().cpu().numpy()
    fc2_b = model.fc2.bias.detach().cpu().numpy()
    # Transpose fc1_weights
    weights = np.concatenate(
        [fc1_w.T.flatten(), fc1_b.flatten(), fc2_w.flatten(), fc2_b.flatten()]
    ).astype(np.float32)
    weights.tofile(path)
    logging.info(f"Saved NNUE weights to {path} ({weights.size} floats)")


def train_nnue(
    fen_eval_path,
    model_save_path,
    batch_size=64,
    epochs=10,
    learning_rate=1e-3,
    weight_decay=1e-6,
):
    model = NNUE().to(DEVICE)
    criterion = nn.MSELoss()
    optimizer = optim.AdamW(
        model.parameters(), lr=learning_rate, weight_decay=weight_decay
    )

    logging.info("Training Configuration")
    logging.info(f"Epochs:             {epochs}")
    logging.info(f"Batch size:         {batch_size}")
    logging.info(f"Learning rate:      {learning_rate}")
    logging.info(f"Weight decay:       {weight_decay}")

    dataset = FenEvalDataset(fen_eval_path)
    dataloader = DataLoader(
        dataset,
        batch_size=batch_size,
        num_workers=0,
    )

    for epoch in range(1, epochs + 1):
        logging.info(f"=== Epoch {epoch}/{epochs} ===")
        total_loss = 0.0
        total_samples = 0
        model.train()
        for batch_inputs, batch_targets in dataloader:
            batch_inputs = batch_inputs.to(DEVICE)
            batch_targets = batch_targets.to(DEVICE)

            optimizer.zero_grad()
            outputs = model(batch_inputs)
            loss = criterion(outputs, batch_targets)
            loss.backward()
            optimizer.step()

            total_loss += loss.item() * batch_inputs.size(0)
            total_samples += batch_inputs.size(0)

        avg_loss = total_loss / total_samples if total_samples > 0 else float("nan")
        logging.info(f"Epoch {epoch} - Avg Loss: {avg_loss:.6f}")

    logging.info("Training complete. Saving model...")
    save_nnue_weights(model, model_save_path)


def main():
    if not torch.backends.mps.is_available():
        raise RuntimeError("MPS backend not available")

    parser = argparse.ArgumentParser(
        description="Train NNUE model using PyTorch Dataset on MPS"
    )
    parser.add_argument(
        "--data", type=str, default="fens_evals.txt", help="Path to fens file"
    )
    parser.add_argument(
        "--model", type=str, default="nnue.bin", help="Where to save NNUE weights"
    )
    parser.add_argument("--batch-size", type=int, default=64)
    parser.add_argument("--epochs", type=int, default=10)
    parser.add_argument("--learning-rate", type=float, default=1e-3)
    parser.add_argument("--decay", type=float, default=1e-6)

    args = parser.parse_args()

    train_nnue(
        fen_eval_path=args.data,
        model_save_path=args.model,
        batch_size=args.batch_size,
        epochs=args.epochs,
        learning_rate=args.learning_rate,
        weight_decay=args.decay,
    )


if __name__ == "__main__":
    main()
