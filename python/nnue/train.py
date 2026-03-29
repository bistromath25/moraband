import argparse
import hashlib
import logging
import math
import os
import subprocess
import time
from pathlib import Path

import numpy as np
import torch
import torch.nn as nn
import torch.optim as optim
from torch.utils.data import DataLoader, IterableDataset

torch.set_float32_matmul_precision("high")

BATCH_SIZE = 8192
LEARNING_RATE = 3e-4
DECAY = 1e-5
MAX_CP = 1000
INPUT_FEATURES = 768
HIDDEN_FEATURES = 256
DEVICE = torch.device("mps")

LATEST_CHECKPOINT = "checkpoint.pth"

PIECE_TO_IDX = {
    "P": 0,
    "N": 1,
    "B": 2,
    "R": 3,
    "Q": 4,
    "K": 5,
    "p": 6,
    "n": 7,
    "b": 8,
    "r": 9,
    "q": 10,
    "k": 11,
}


def square_index(file_char, rank_char):
    file = ord(file_char) - ord("a")
    rank = int(rank_char) - 1
    return rank * 8 + file


def encode_fen(fen):
    x = np.zeros(INPUT_FEATURES, dtype=np.float32)

    board_part = fen.split(" ")[0]
    ranks = board_part.split("/")

    for r, row in enumerate(ranks):
        rank = 7 - r
        file = 0

        for c in row:
            if c.isdigit():
                file += int(c)
            else:
                sq = rank * 8 + file
                idx = sq * 12 + PIECE_TO_IDX[c]
                x[idx] = 1.0
                file += 1

    return x


def format_time(seconds):
    minutes, sec = divmod(int(seconds), 60)
    return f"{minutes:02d}:{sec:02d}"


def get_checkpoint_filename(epoch):
    return f"checkpoint-hidden{HIDDEN_FEATURES}-epoch{epoch}.pth"


def load_checkpoint(model, optimizer, path):
    checkpoint = torch.load(path, map_location=DEVICE)

    model.load_state_dict(checkpoint["model_state"])
    optimizer.load_state_dict(checkpoint["optimizer_state"])

    start_epoch = checkpoint["epoch"] + 1

    logging.info(f"Loaded checkpoint: {path}")
    logging.info(f"Resuming from epoch {start_epoch}")

    return start_epoch


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
                    fen, score_str = [s.strip() for s in line.split(";")]
                    x = encode_fen(fen)

                    raw_eval = int(score_str)
                    clamped = (
                        max(-self.max_cp, min(self.max_cp, raw_eval)) / self.max_cp
                    )

                    yield (
                        torch.from_numpy(x),
                        torch.tensor([clamped], dtype=torch.float32),
                    )

                except Exception as e:
                    logging.warning(f"Skipping line {line} due to error: {e}")
                    continue


def save_nnue_weights(model, output_dir):
    fc1_w = model.fc1.weight.detach().cpu().numpy()
    fc1_b = model.fc1.bias.detach().cpu().numpy()
    fc2_w = model.fc2.weight.detach().cpu().numpy()
    fc2_b = model.fc2.bias.detach().cpu().numpy()
    # Transpose fc1_weights
    weights = np.concatenate(
        [fc1_w.T.flatten(), fc1_b.flatten(), fc2_w.flatten(), fc2_b.flatten()]
    ).astype(np.float32)
    digest = hashlib.sha256(weights.tobytes()).hexdigest()[:6]
    filename = output_dir / f"nn-{digest}.nnue"
    weights.tofile(filename)
    logging.info(f"Saved NNUE weights to {filename} ({weights.size} floats)")


def train_nnue(
    fen_eval_path,
    output_dir,
    batch_size=BATCH_SIZE,
    epochs=10,
    learning_rate=LEARNING_RATE,
    weight_decay=DECAY,
    resume=False,
    resume_epoch=None,
):
    checkpoint_dir = output_dir / "checkpoints"
    checkpoint_dir.mkdir(parents=True, exist_ok=True)

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
    print_every = max(1, total_batches // 20)

    logging.info("Training Configuration")
    logging.info(f"Output directory:   {output_dir}")
    logging.info(f"Epochs:             {epochs}")
    logging.info(f"Batch size:         {batch_size}")
    logging.info(f"Learning rate:      {learning_rate}")
    logging.info(f"Weight decay:       {weight_decay}")
    logging.info(f"Total lines:        {total_samples}")
    logging.info(f"Total batches:      {total_batches}")

    path = None
    if resume_epoch:
        path = checkpoint_dir / get_checkpoint_filename(resume_epoch)
    elif resume:
        path = checkpoint_dir / LATEST_CHECKPOINT
    if path and os.path.exists(path):
        start_epoch = load_checkpoint(model, optimizer, path)

    dataset = FenEvalDataset(fen_eval_path)
    dataloader = DataLoader(dataset, batch_size=batch_size)

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

            if batch_idx % print_every == 0 or batch_idx == total_batches:
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
        checkpoint = {
            "epoch": epoch,
            "model_state": model.state_dict(),
            "optimizer_state": optimizer.state_dict(),
        }
        torch.save(
            checkpoint,
            checkpoint_dir / get_checkpoint_filename(epoch),
        )
        torch.save(
            checkpoint,
            checkpoint_dir / LATEST_CHECKPOINT,
        )
        logging.info(f"Checkpoint saved at epoch {epoch}")

    logging.info("Training complete. Saving model...")
    save_nnue_weights(model, output_dir)


def main():
    parser = argparse.ArgumentParser(
        description="Train NNUE model using PyTorch Dataset on MPS"
    )
    parser.add_argument("--data", type=str, required=True, help="Path to fens file")
    parser.add_argument(
        "--output-dir",
        type=str,
        help="Directory to write outputs",
    )
    parser.add_argument("--batch-size", type=int, default=BATCH_SIZE)
    parser.add_argument("--epochs", type=int, default=10)
    parser.add_argument("--learning-rate", type=float, default=LEARNING_RATE)
    parser.add_argument("--decay", type=float, default=DECAY)
    parser.add_argument(
        "--resume", action="store_true", help="Resume training from checkpoint"
    )
    parser.add_argument(
        "--resume-epoch", type=int, default=None, help="Resume training from epoch"
    )
    args = parser.parse_args()

    if args.output_dir is None:
        timestamp = time.strftime("%Y%m%d_%H%M%S")
        args.output_dir = f"train-{timestamp}"

    output_dir = Path(args.output_dir)
    output_dir.mkdir(parents=True, exist_ok=True)

    logging.basicConfig(
        filename=output_dir / "train.log",
        level=logging.INFO,
        format="[%(asctime)s] %(levelname)s: %(message)s",
    )

    train_nnue(
        fen_eval_path=args.data,
        output_dir=output_dir,
        batch_size=args.batch_size,
        epochs=args.epochs,
        learning_rate=args.learning_rate,
        weight_decay=args.decay,
        resume=args.resume,
        resume_epoch=args.resume_epoch,
    )


if __name__ == "__main__":
    main()
