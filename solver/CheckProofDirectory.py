#!/usr/bin/env python3

import os
import re
import sys

HEX_PAIR_REGEX = re.compile(r"^[0-9a-f]{2}$")
PROOF_FILENAME_REGEX = re.compile(r"^[0-9a-f]{64}\.txt$")


def valid_shard_dir(shard_dir: os.DirEntry[str]) -> bool:
    return shard_dir.is_dir(follow_symlinks=False) and (
        HEX_PAIR_REGEX.fullmatch(shard_dir.name) is not None
    )


def valid_proof_file(proof_file: os.DirEntry[str]) -> bool:
    return proof_file.is_file(follow_symlinks=False) and (
        PROOF_FILENAME_REGEX.fullmatch(proof_file.name) is not None
    )


def clear_progress():
    print("\r\033[K", end="", file=sys.stderr, flush=True)


def print_progress(message: str):
    print("\r" + message, end="", file=sys.stderr, flush=True)


def main():
    n = 0
    with os.scandir("proofs") as shard1_iterator:
        for shard1_dir in shard1_iterator:
            if not valid_shard_dir(shard1_dir):
                clear_progress()
                print(shard1_dir.path, flush=True)
                continue
            with os.scandir(shard1_dir.path) as shard2_iterator:
                for shard2_dir in shard2_iterator:
                    if not valid_shard_dir(shard2_dir):
                        clear_progress()
                        print(shard2_dir.path, flush=True)
                        continue
                    with os.scandir(shard2_dir.path) as proof_iterator:
                        for proof_file in proof_iterator:
                            if (
                                not valid_proof_file(proof_file)
                                or proof_file.name[0:2] != shard1_dir.name
                                or proof_file.name[2:4] != shard2_dir.name
                            ):
                                clear_progress()
                                print(proof_file.path, flush=True)
                            n += 1
                            if n % 100_000 == 0:
                                print_progress(f"Found {n} proof files.")
    clear_progress()
    print(f"Found {n} proof files in total.", file=sys.stderr, flush=True)


if __name__ == "__main__":
    main()
