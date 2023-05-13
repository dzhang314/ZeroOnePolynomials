#!/usr/bin/env python3

import itertools
import os
import subprocess


def pair_iterator():
    for k in itertools.count():
        print(k)
        for j in range(k + 1):
            i = k - j
            if i > j > 0:
                yield (i, j)


NUM_PROCESSES = os.cpu_count()
if NUM_PROCESSES is None:
    NUM_PROCESSES = 8  # reasonable guess for number of cores
NUM_PROCESSES -= 1  # reserve one process for this program, desktop UI, etc.


processes = []
for i, j in pair_iterator():
    filename = f"data/ReducedEquations_{i:04}_{j:04}.txt"
    # if this data file has not already been computed...
    if os.path.isfile(filename):
        print(filename, "already computed.")
    else:
        # if we have hit the process limit, wait for a process to finish
        while len(processes) >= NUM_PROCESSES:
            for k in range(NUM_PROCESSES):
                if processes[k].poll() is not None:
                    del processes[k]
                    break
        # now that a slot is available, start a process to compute this file
        processes.append(
            subprocess.Popen(["./EquationReducer", str(i), str(j), filename])
        )
