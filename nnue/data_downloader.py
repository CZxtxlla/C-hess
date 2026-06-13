import chess
import struct
import math
from datasets import load_dataset

OUTPUT_FILE = "data.bin"
POSITIONS_TO_DOWNLOAD = 15_000_000 # Stop after 2 million positions

def piece_to_int(piece):
    if piece is None: return 0
    offset = 0 if piece.color == chess.WHITE else 6
    return piece.piece_type + offset

def board_to_bytes(board):
    board_data = bytearray([0] * 64)
    for sq, piece in board.piece_map().items():
        board_data[sq] = piece_to_int(piece)
    return board_data

def get_win_probability(centipawns):
    cp = max(-10000, min(10000, centipawns))
    return 1.0 / (1.0 + math.exp(-cp / 400.0))

def main():
    print("Connecting to Hugging Face stream...")
    
    # streaming=True is the magic parameter. It means "don't download the file, 
    # just stream the rows over the internet one by one."
    dataset = load_dataset("lukesalamone/gigafish-3.8b-d10", split="train", streaming=True)

    total_converted = 0

    with open(OUTPUT_FILE, "ab") as bin_out:
        # Iterate over the cloud dataset row by row
        for row in dataset:
            if total_converted >= POSITIONS_TO_DOWNLOAD:
                break

            try:
                # Gigafish columns are typically 'fen' and 'score'
                fen = row['fen']
                eval_str = str(row['eval']) 

                board = chess.Board(fen)
                board_bytes = board_to_bytes(board)

                # Parse the score (Gigafish might format mate scores as string "#3")
                if eval_str.startswith('#'):
                    mate_in = int(eval_str.replace('#', ''))
                    cp_eval = 10000 if mate_in > 0 else -10000
                else:
                    cp_eval = float(eval_str)
                    
                eval_prob = get_win_probability(cp_eval)

                # Because Gigafish doesn't have game results (WDL), we use pure Supervised Learning.
                # So our target is JUST the evaluation probability (Lambda = 1.0)
                blended_target = eval_prob

                # Write to our blazing-fast C dataloader format
                bin_out.write(board_bytes)
                bin_out.write(struct.pack("f", blended_target))

                total_converted += 1
                
                if total_converted % 50000 == 0:
                    print(f"Streamed and converted {total_converted} positions...")

            except Exception as e:
                # Ignore malformed rows if any exist
                pass

    print(f"\nSuccess! Built {OUTPUT_FILE} with {total_converted} positions.")

if __name__ == "__main__":
    main()