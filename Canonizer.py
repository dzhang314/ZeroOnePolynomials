#!/usr/bin/env python3

from collections.abc import Iterator
from itertools import chain, count
from os.path import isfile
from subprocess import run
from tempfile import NamedTemporaryFile


def line_block_iterator(filename: str) -> Iterator[list[str]]:
    with open(filename) as f:
        result: list[str] = []
        for line in f:
            line = line.strip()
            if line:
                result.append(line)
            else:
                yield result
                result = []
        assert not result


def pair_iterator(k: int) -> Iterator[tuple[int, int]]:
    for j in range(k + 1):
        i = k - j
        if i > j > 0:
            yield (i, j)


def files_available(k: int) -> bool:
    for i, j in pair_iterator(k):
        if not isfile(f"data/ZeroOneEquations_{i:04}_{j:04}.txt"):
            return False
    return True


def count_available() -> int:
    for k in count():
        if not files_available(k):
            return k - 1
    assert False  # unreachable


def equation_system_iterator(k: int) -> Iterator[list[str]]:
    assert files_available(k)
    return chain(
        *[
            line_block_iterator(f"data/ZeroOneEquations_{i:04}_{j:04}.txt")
            for i, j in pair_iterator(k)
        ]
    )


Variable = tuple[str, int]
Term = tuple[Variable, ...]
Equation = tuple[Term, ...]
System = tuple[Equation, ...]
Signature = tuple[tuple[int, int], ...]


def variable_from_str(var: str) -> Variable:
    prefix, index = var.split("_")
    return (prefix, int(index))


def term_from_str(term: str) -> Term:
    return tuple(variable_from_str(var.strip()) for var in term.split("*"))


def system_from_strs(system: list[str]) -> System:
    return tuple(
        tuple(term_from_str(term.strip()) for term in equation.split("+"))
        for equation in system
    )


def sort_equation_system(system: System) -> System:
    tagged_equations: list[tuple[tuple[int, int], Equation]] = []
    for equation in system:
        linear_terms = [term for term in equation if len(term) == 1]
        quadratic_terms = [tuple(sorted(term)) for term in equation if len(term) == 2]
        assert len(linear_terms) + len(quadratic_terms) == len(equation)
        tagged_equations.append(
            (
                (len(quadratic_terms), len(linear_terms)),
                tuple(sorted(linear_terms) + sorted(quadratic_terms)),
            )
        )
    return tuple(equation for _, equation in sorted(tagged_equations))


def rename_variables(system: System) -> System:
    new_variables: dict[Variable, Variable] = {}
    for equation in system:
        for term in equation:
            for var in term:
                if var not in new_variables:
                    new_variables[var] = ("x", len(new_variables) + 1)
    return tuple(
        tuple(tuple(new_variables[var] for var in term) for term in equation)
        for equation in system
    )


def canonize(system: System) -> System:
    system = sort_equation_system(rename_variables(sort_equation_system(system)))
    while True:
        next_system = sort_equation_system(rename_variables(system))
        if next_system == system:
            return system
        else:
            system = next_system


def canonized_equation_system_iterator(k_max: int) -> Iterator[System]:
    seen: set[System] = set()
    for k in range(k_max + 1):
        for system in equation_system_iterator(k):
            canonized = canonize(system_from_strs(system))
            if canonized not in seen:
                yield canonized
                seen.add(canonized)


def signature(system: System) -> Signature:
    result: list[tuple[int, int]] = []
    for equation in system:
        num_linear_terms = 0
        num_quadratic_terms = 0
        for term in equation:
            if len(term) == 1:
                num_linear_terms += 1
            elif len(term) == 2:
                num_quadratic_terms += 1
        assert num_linear_terms + num_quadratic_terms == len(equation)
        result.append((num_quadratic_terms, num_linear_terms))
    return tuple(sorted(result))


# SUBSCRIPT_TABLE = str.maketrans(
#     "0123456789", "\u2080\u2081\u2082\u2083\u2084\u2085\u2086\u2087\u2088\u2089"
# )


def macaulay_file(system: System) -> str:
    vars: list[Variable] = sorted(
        set(var for equation in system for term in equation for var in term)
    )
    var_list: str = ", ".join(f"{prefix}_{index}" for prefix, index in vars)
    equation_list: str = ", ".join(
        " + ".join(
            "*".join(f"{prefix}_{index}" for prefix, index in term) for term in equation
        )
        + " - 1"
        for equation in system
    )
    return f"R = QQ[{var_list}]\nI = ideal({equation_list})\nprint(1 % I)\n"


def verify_system(system: System) -> bool:
    with NamedTemporaryFile() as temp:
        temp.write(macaulay_file(system).encode("utf-8"))
        temp.flush()
        process = run(["M2", "--script", temp.name], check=True, capture_output=True)
        correct_stdout = b"0\n"
        correct_stderr = b""
        if (process.stdout == correct_stdout) and (process.stderr == correct_stderr):
            return True
    print("FOUND COUNTEREXAMPLE:", system)
    return False


def verify_block(block: list[System]) -> bool:
    with NamedTemporaryFile() as temp:
        for system in block:
            temp.write(macaulay_file(system).encode("utf-8"))
        temp.flush()
        process = run(["M2", "--script", temp.name], check=True, capture_output=True)
        correct_stdout = b"0\n" * len(block)
        correct_stderr = b""
        if (process.stdout == correct_stdout) and (process.stderr == correct_stderr):
            return True
    for system in block:
        if not verify_system(system):
            return False
    assert False  # unreachable


def main():
    k_max: int = count_available()
    print("Files for degree <=", k_max, "are available.")
    count: int = 0
    block: list[System] = []
    for system in canonized_equation_system_iterator(k_max):
        if len(block) >= 500:
            verify_block(block)
            count += len(block)
            print("Verified", count, "systems.")
            block.clear()
        block.append(system)
    verify_block(block)


if __name__ == "__main__":
    main()
