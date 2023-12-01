#!/usr/bin/env python3

import os
import subprocess
from collections.abc import Iterator
from itertools import count
from time import sleep
from sys import argv


def degree_pair_iterator(degree: int) -> Iterator[tuple[int, int]]:
    """
    Given an integer degree, return an iterator over all pairs
    of integers (m, n) such that 0 < m < n and m + n == degree.
    """
    for m in range(1, (degree + 1) >> 1):
        n = degree - m
        yield (m, n)


def max_degree_pair_iterator(max_degree: int | None) -> Iterator[tuple[int, int]]:
    """
    Given an integer max_degree, return an iterator over all pairs
    of integers (m, n) such that 0 < m < n and m + n <= max_degree.

    The pairs are returned in increasing order of m + n. If max_degree is
    None, it is treated as infinity, and an infinite iterator is returned.
    """
    for degree in count() if max_degree is None else range(max_degree + 1):
        yield from degree_pair_iterator(degree)


def get_num_cores() -> int:
    """
    Return the number of logical CPU cores in the machine executing this
    program (or a reasonable guess, 4, if this information is not available.)
    """
    result = os.cpu_count()
    return 4 if result is None else result


def executable_path(m: int, n: int) -> str:
    return f"bin/ZeroOneSolver-{m+n:04}-{m:04}-{n:04}"


def data_file_path(m: int, n: int) -> str:
    return f"data/ZeroOneEquations-{m+n:04}-{m:04}-{n:04}.txt"


def compile(m: int, n: int, output_path: str):
    if os.path.isfile(output_path):
        os.remove(output_path)
    subprocess.run(
        [
            "g++",
            "-std=c++20",
            "-O3",
            "-march=native",
            "-DZERO_ONE_SOLVER_M=" + str(m),
            "-DZERO_ONE_SOLVER_N=" + str(n),
            "-DZERO_ONE_SOLVER_VERBOSE=false",
            "ZeroOneSolver.cpp",
            "-o",
            output_path,
        ],
        check=True,
    )


def wait_for_process_to_finish(
    processes: list[tuple[subprocess.Popen[bytes], str, str, str]]
):
    while True:
        for k in range(len(processes)):
            proc, exe, src, dst = processes[k]
            return_code = proc.poll()
            if return_code is not None:
                if return_code == 0:
                    print("Finished computing", dst + ".")
                else:
                    print("ERROR: Process", dst, "returned non-zero exit code.")
                if os.path.isfile(exe):
                    os.remove(exe)
                if os.path.isfile(exe + ".exe"):
                    os.remove(exe + ".exe")
                if proc.stdout is not None:
                    proc.stdout.close()
                os.rename(src, dst)
                del processes[k]
                return None
        sleep(0.001)


def main():
    if not os.path.isdir("bin"):
        os.mkdir("bin")
    if not os.path.isdir("data"):
        os.mkdir("data")

    num_processes = int(argv[1]) if len(argv) > 1 else get_num_cores() - 1
    print("Running", num_processes, "parallel processes.")

    processes: list[tuple[subprocess.Popen[bytes], str, str, str]] = []
    for m, n in max_degree_pair_iterator(int(argv[2]) if len(argv) > 2 else None):
        data_path = data_file_path(m, n)
        if os.path.isfile(data_path):
            print(data_path, "already computed.")
        else:
            while len(processes) >= num_processes:
                wait_for_process_to_finish(processes)
            exe_path = executable_path(m, n)
            compile(m, n, exe_path)
            temp_path = data_path + ".temp"
            print("Computing", data_path + ".")
            processes.append(
                (
                    subprocess.Popen([exe_path], stdout=open(temp_path, "w")),
                    exe_path,
                    temp_path,
                    data_path,
                )
            )
    while processes:
        wait_for_process_to_finish(processes)


if __name__ == "__main__":
    main()
