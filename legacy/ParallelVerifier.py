#!/usr/bin/env python3

import glob
import os
import subprocess
import sys
import time


def get_num_processes() -> int:
    result = os.cpu_count()
    if result is None:
        result = 8  # reasonable guess for number of cores
    return result // 2  # Macaulay2 seems to use two threads per process


def find_finished_process(processes: list[subprocess.Popen[bytes]]) -> int | None:
    for k in range(len(processes)):
        if processes[k].poll() is not None:
            return k
    return None


def finalize(processes: list[subprocess.Popen[bytes]], k: int):
    assert 0 <= k < len(processes)
    args = processes[k].args
    assert isinstance(args, list)
    return_code = processes[k].poll()
    assert return_code is not None
    stderr = processes[k].stderr
    assert stderr is not None
    stdout = processes[k].stdout
    assert stdout is not None
    if return_code == 0:
        assert stderr.read() == b""
        if all(line == b"0" for line in stdout.read().splitlines()):
            print("Finished verifying:", args)
            src = args[2]
            assert isinstance(src, str)
            dst = os.path.join("verified", *os.path.split(src)[1:])
            os.rename(src, dst)
        else:
            print("FOUND COUNTEREXAMPLE IN:", args)
    else:
        print("ERROR: Process", args, "did not execute successfully.")
    del processes[k]


def main():
    processes: list[subprocess.Popen[bytes]] = []
    num_processes = int(sys.argv[1]) if len(sys.argv) > 1 else get_num_processes()
    for filename in sorted(glob.glob("data/CanonizedEquationBlock_*.m2")):
        while len(processes) >= num_processes:
            k = find_finished_process(processes)
            if k is None:
                time.sleep(0.25)
            else:
                finalize(processes, k)
        args = ["M2", "--script", filename]
        print("Starting process:", args)
        processes.append(
            subprocess.Popen(
                args,
                stdin=None,
                stdout=subprocess.PIPE,
                stderr=subprocess.PIPE,
            )
        )
    while processes:
        k = find_finished_process(processes)
        if k is not None:
            finalize(processes, k)


if __name__ == "__main__":
    main()
