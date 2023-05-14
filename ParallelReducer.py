#!/usr/bin/env python3

import itertools
import os
import subprocess
import sys


def pair_iterator():
    for k in itertools.count():
        for j in range(k + 1):
            i = k - j
            if i > j > 0:
                yield (i, j)


def get_num_cores() -> int:
    result = os.cpu_count()
    if result is None:
        result = 8  # reasonable guess for number of cores
    return result - 1  # reserve one process for this program, desktop UI, etc.


def main():
    processes: list[subprocess.Popen[bytes]] = []
    num_processes = int(sys.argv[1]) if len(sys.argv) > 1 else get_num_cores()
    for i, j in pair_iterator():
        filename = f"data/DeepReducedEquations_{i:04}_{j:04}.txt"
        # if this data file has not already been computed...
        if os.path.isfile(filename):
            print(filename, "already computed.")
        else:
            # if we have hit the process limit, wait for a process to finish
            while len(processes) >= num_processes:
                for k in range(len(processes)):
                    if processes[k].poll() is not None:
                        del processes[k]
                        break
            # now that a slot is available, start a process to compute this file
            processes.append(
                subprocess.Popen(["./DeepEquationReducer", str(i), str(j), filename])
            )


if __name__ == "__main__":
    main()
