import io
import itertools
import os
import subprocess
import sys
import time


def pair_iterator(degree: int):
    for m in range(1, (degree + 1) >> 1):
        n = degree - m
        yield (m, n)


def all_pair_iterator(max_degree: int | None):
    for degree in itertools.count() if max_degree is None else range(max_degree + 1):
        yield from pair_iterator(degree)


def get_num_cores() -> int:
    result = os.cpu_count()
    if result is None:
        result = 4  # reasonable guess for number of cores
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
                    print("Finished computing", dst + ".")
                else:
                    print("ERROR: Process", dst, "returned non-zero exit code.")
                del processes[k]
                file.close()
                os.rename(src, dst)
                return None
        time.sleep(0.001)


def compile(m: int, n: int, output_path: str):
    if os.path.isfile(output_path):
        os.remove(output_path)
    subprocess.run(
        [
            "gcc",
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


def main():
    if not os.path.isdir("bin"):
        os.mkdir("bin")
    if not os.path.isdir("data"):
        os.mkdir("data")
    processes: list[tuple[subprocess.Popen[bytes], io.TextIOWrapper, str, str]] = []
    num_processes = int(sys.argv[1]) if len(sys.argv) > 1 else get_num_cores()
    for m, n in all_pair_iterator(int(sys.argv[2]) if len(sys.argv) > 2 else None):
        data_name = f"data/ZeroOneEquations-{m+n:04}-{m:04}-{n:04}.txt"
        if os.path.isfile(data_name):
            print(data_name, "already computed.")
        else:
            while len(processes) >= num_processes:
                wait_for_process_to_finish(processes)
            binary_name = f"bin/ZeroOneSolver-{m}-{n}"
            compile(m, n, binary_name)
            temp_name = data_name + ".temp"
            temp_file = open(temp_name, "w")
            print("Computing", data_name + ".")
            processes.append(
                (
                    subprocess.Popen([f"bin/ZeroOneSolver-{m}-{n}"], stdout=temp_file),
                    temp_file,
                    temp_name,
                    data_name,
                )
            )
    while processes:
        wait_for_process_to_finish(processes)


if __name__ == "__main__":
    main()
