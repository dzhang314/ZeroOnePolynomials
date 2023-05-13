#!/usr/bin/env python3

import itertools
import subprocess


def pair_iterator():
    for i in itertools.count(start=1):
        for j in range(1, i):
            yield (i, j)


NUM_PROCESSES = 15
processes = []


for i, j in pair_iterator():
    while len(processes) >= NUM_PROCESSES:
        for k in range(NUM_PROCESSES):
            if processes[k].poll() is not None:
                del processes[k]
                break
    processes.append(
        subprocess.Popen(
            [
                "./EquationReducer",
                str(i),
                str(j),
                f"data/ReducedEquations_{i:04}_{j:04}.txt",
            ]
        )
    )
