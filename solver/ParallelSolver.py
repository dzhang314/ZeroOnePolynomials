#!/usr/bin/env python3

import os
from collections.abc import Iterator
from itertools import count
from time import perf_counter, sleep
from shutil import copyfileobj
from subprocess import Popen, run
from sys import argv, stderr


def get_num_cores() -> int:
    """
    Return the number of logical CPU cores in the machine executing this
    program (or a reasonable guess, 8, if this information is not available.)
    """
    result: int | None = os.cpu_count()
    return 8 if result is None else result


def find_gcc_executable() -> str:
    """
    Return the path to the most recent version of the GNU C++ compiler
    installed on the system. If no such compiler is found, return "g++".
    """
    for bin_dir in ["/opt/homebrew/bin", "/usr/bin"]:
        if os.path.isdir(bin_dir):
            candidates: list[tuple[int, str]] = []
            for filename in os.listdir(bin_dir):
                if filename.startswith("g++-"):
                    try:
                        candidates.append((int(filename[4:]), filename))
                    except ValueError:
                        pass
            if candidates:
                return os.path.join(bin_dir, sorted(candidates)[-1][1])
    return "g++"


NUM_CORES: int = get_num_cores()
GCC_EXECUTABLE: str = find_gcc_executable()


def degree_pair_iterator(d: int) -> Iterator[tuple[int, int]]:
    """
    Given an integer d, return an iterator over all pairs of integers
    (m, n) such that 0 < m < n, m + n == d, and m == 5 or m >= 7.
    """
    # The cases m == 1, 2, 3, 4, and 6 have already been solved for all n,
    # so we only need to consider m == 5 and m >= 7.
    for m in range(5, (d + 1) >> 1):
        if m == 5 or m >= 7:
            n: int = d - m
            yield (m, n)


def executable_path(m: int, n: int) -> str:
    return os.path.join("bin", f"ZeroOneSolver-{m+n:04}-{m:04}-{n:04}")


def data_file_path(m: int, n: int) -> str:
    return os.path.join("data", f"ZeroOneEquations-{m+n:04}-{m:04}-{n:04}.txt")


def chunk_file_path(m: int, n: int, chunk_index: int) -> str:
    return os.path.join(
        "data", f"ZeroOneEquations-{m+n:04}-{m:04}-{n:04}.{chunk_index+1:08}.txt"
    )


def compile_zero_one_solver(m: int, n: int, optimize: bool) -> None:
    exe_path: str = executable_path(m, n)
    if os.path.isfile(exe_path):
        print("Recompiling", executable_path(m, n) + "...", flush=True)
        os.remove(exe_path)
    else:
        print("Compiling", executable_path(m, n) + "...", flush=True)
    compile_command: list[str] = [GCC_EXECUTABLE, "-std=c++20"]
    if optimize:
        compile_command.extend(["-O3", "-march=native", "-fwhole-program"])
    compile_command.append("-DZERO_ONE_SOLVER_M=" + str(m))
    compile_command.append("-DZERO_ONE_SOLVER_N=" + str(n))
    compile_command.extend(["ZeroOneSolver.cpp", "-o", exe_path])
    _ = run(compile_command, check=True)
    assert os.path.isfile(exe_path) or os.path.isfile(exe_path + ".exe")


CHUNK_SIZE: int = 1 << 16


def wait_for_process_to_finish(
    processes: list[tuple[Popen[bytes], tuple[int, int, int]]],
) -> None:
    while True:
        for k in range(len(processes)):
            process, (m, n, chunk_index) = processes[k]
            return_code: int | None = process.poll()
            if return_code is not None:
                if process.stdout is not None:
                    process.stdout.close()
                if return_code == 0:
                    print(f"Finished computing chunk {chunk_index+1:08}.", flush=True)
                    chunk_path: str = chunk_file_path(m, n, chunk_index)
                    os.rename(chunk_path + ".temp", chunk_path)
                else:
                    lower_bound: int = chunk_index * CHUNK_SIZE
                    upper_bound: int = (chunk_index + 1) * CHUNK_SIZE
                    print(
                        "ERROR:",
                        executable_path(m, n),
                        lower_bound,
                        upper_bound,
                        "produced nonzero return code:",
                        return_code,
                        file=stderr,
                        flush=True,
                    )
                del processes[k]
                return
        sleep(0.001)


def run_zero_one_solver(m: int, n: int, num_processes: int) -> None:
    exe_path: str = executable_path(m, n)
    assert os.path.isfile(exe_path)
    num_items: int = 1 << (m - 1)
    num_chunks: int = max(1, num_items // CHUNK_SIZE)
    processes: list[tuple[Popen[bytes], tuple[int, int, int]]] = []
    for chunk_index in range(num_chunks):
        chunk_path: str = chunk_file_path(m, n, chunk_index)
        if os.path.isfile(chunk_path):
            print(chunk_path, "already exists.", flush=True)
            continue
        lower_bound: int = chunk_index * CHUNK_SIZE
        print(f"Computing chunk {chunk_index+1:8} of {num_chunks:8}...", flush=True)
        processes.append(
            (
                Popen(
                    [exe_path, f"{lower_bound:b}", str(CHUNK_SIZE)],
                    stdout=open(chunk_path + ".temp", "w"),
                ),
                (m, n, chunk_index),
            )
        )
        while len(processes) >= num_processes:
            wait_for_process_to_finish(processes)
    while processes:
        wait_for_process_to_finish(processes)
    os.remove(exe_path)


def merge_chunk_files(m: int, n: int) -> None:
    data_path: str = data_file_path(m, n)
    assert not os.path.isfile(data_path)
    num_items: int = 1 << (m - 1)
    num_chunks: int = max(1, num_items // CHUNK_SIZE)
    with open(data_path, "w") as data_file:
        for chunk_index in range(num_chunks):
            chunk_path = chunk_file_path(m, n, chunk_index)
            assert os.path.isfile(chunk_path)
            with open(chunk_path) as chunk_file:
                copyfileobj(chunk_file, data_file)
            os.remove(chunk_path)


def main() -> None:
    if not os.path.isdir("bin"):
        os.mkdir("bin")
    if not os.path.isdir("data"):
        os.mkdir("data")
    num_processes: int = int(argv[1]) if len(argv) > 1 else NUM_CORES
    print("Running", num_processes, "parallel processes.", flush=True)
    optimize = False
    for d in count():
        for m, n in degree_pair_iterator(d):
            data_path = data_file_path(m, n)
            if os.path.isfile(data_path):
                print(data_path, "already computed.", flush=True)
                continue
            compile_start: float = perf_counter()
            compile_zero_one_solver(m, n, optimize)
            compile_time: float = perf_counter() - compile_start
            print(f"Compiled in {compile_time:.3f} seconds.", flush=True)
            solve_start: float = perf_counter()
            run_zero_one_solver(m, n, num_processes)
            solve_time: float = perf_counter() - solve_start
            print(f"Computed in {solve_time:.3f} seconds.", flush=True)
            merge_chunk_files(m, n)
            if solve_time >= compile_time:
                optimize = True


if __name__ == "__main__":
    main()
