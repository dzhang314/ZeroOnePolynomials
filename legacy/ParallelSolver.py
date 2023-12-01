#!/usr/bin/env python3

import io
import itertools
import os
import subprocess
import sys
import time


def pair_iterator(n: int | None):
    for k in itertools.count() if n is None else range(n + 1):
        for j in range(k + 1):
            i = k - j
            if i > j > 0:
                yield (i, j)


def get_num_cores() -> int:
    result = os.cpu_count()
    if result is None:
        result = 8  # reasonable guess for number of cores
    return result - 1  # reserve one process for this program, desktop UI, etc.


def wait_for_process_to_finish(
    processes: list[tuple[subprocess.Popen[bytes], io.TextIOWrapper, str, str]]
):
    while True:
        for k in range(len(processes)):
            return_code = processes[k][0].poll()
            if return_code is not None:
                _, file, src, dst = processes[k]
                if return_code == 0:
                    print("Finished computation of", dst + ".")
                else:
                    print("ERROR: Process", dst, "returned non-zero exit code.")
                del processes[k]
                file.close()
                os.rename(src, dst)
                return None
        time.sleep(0.125)


def main():
    if not os.path.isdir("data"):
        os.mkdir("data")
    processes: list[tuple[subprocess.Popen[bytes], io.TextIOWrapper, str, str]] = []
    num_processes = int(sys.argv[1]) if len(sys.argv) > 1 else get_num_cores()
    for i, j in pair_iterator(int(sys.argv[2]) if len(sys.argv) > 2 else None):
        file_name = f"data/ZeroOneEquations_{i:04}_{j:04}.txt"
        # if this data file has not already been computed...
        if os.path.isfile(file_name):
            print(file_name, "already computed.")
        else:
            # if we have hit the process limit, wait for a process to finish
            while len(processes) >= num_processes:
                wait_for_process_to_finish(processes)
            # now that a slot is available, start a process to compute this file
            temp_name = file_name + "." + str(i + j) + ".temp"
            file = open(temp_name, "w+")
            print("Starting computation of", file_name + ".")
            processes.append(
                (
                    subprocess.Popen(
                        ["bin/ZeroOneSolver", str(i), str(j), "--paranoid"], stdout=file
                    ),
                    file,
                    temp_name,
                    file_name,
                )
            )
    while processes:
        wait_for_process_to_finish(processes)


if __name__ == "__main__":
    main()
