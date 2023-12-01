#!/usr/bin/env python3

import os
from collections.abc import Iterator
from sys import stderr

from ParallelSolver import data_file_path, degree_pair_iterator


def line_block_iterator(path: str) -> Iterator[list[str]]:
    """
    Split a text file into multiline blocks terminated by empty lines
    and return an iterator that yields each block as a list of strings.

    The final block is expected to be terminated by an empty line, so
    the file should end with two consecutive newline characters ("\n\n").
    """
    with open(path) as file:
        result: list[str] = []
        for line in file:
            if line.endswith("\n"):
                line = line[:-1]
            if line:
                result.append(line)
            else:
                yield result
                result = []
        assert not result


def data_files_available(degree: int) -> bool:
    for m, n in degree_pair_iterator(degree):
        if not os.path.isfile(data_file_path(m, n)):
            return False
    return True


def equation_block_iterator(degree: int) -> Iterator[list[str]]:
    assert data_files_available(degree)
    for m, n in degree_pair_iterator(degree):
        yield from line_block_iterator(data_file_path(m, n))


Variable = tuple[str, int]
Term = tuple[Variable, ...]
Polynomial = tuple[Term, ...]
System = tuple[Polynomial, ...]


def parse_variable(var: str) -> Variable:
    return (var[0], int(var[1:]))


def parse_system(block: list[str]) -> System:
    return tuple(
        tuple(
            tuple(parse_variable(variable.strip()) for variable in term.split("*"))
            for term in polynomial.split("+")
        )
        for polynomial in block
    )


def sorted_system(system: System) -> System:
    tagged_equations: list[tuple[tuple[int, int], Polynomial]] = []
    for polynomial in system:
        linear_terms = [term for term in polynomial if len(term) == 1]
        quadratic_terms = [tuple(sorted(term)) for term in polynomial if len(term) == 2]
        assert len(linear_terms) + len(quadratic_terms) == len(polynomial)
        tag = (len(quadratic_terms), len(linear_terms))
        sorted_equation: Polynomial = tuple(
            sorted(quadratic_terms) + sorted(linear_terms)
        )
        tagged_equations.append((tag, sorted_equation))
    return tuple(polynomial for _, polynomial in sorted(tagged_equations))


def rename_variables(system: System) -> System:
    new_variables: dict[Variable, Variable] = {}
    indices: dict[str, int] = {}
    for polynomial in system:
        for term in polynomial:
            for label, index in term:
                if (label, index) not in new_variables:
                    if label in indices:
                        indices[label] += 1
                    else:
                        indices[label] = 1
                    new_variables[(label, index)] = (label, indices[label])
    return tuple(
        tuple(tuple(new_variables[var] for var in term) for term in polynomial)
        for polynomial in system
    )


def canonize(system: System) -> System:
    system = sorted_system(system)
    while True:
        next_system = sorted_system(rename_variables(system))
        if next_system == system:
            return system
        else:
            system = next_system


def wolfram_string(system: System) -> str:
    return "\n".join(
        " + ".join(
            " ".join(f"{label}[{index}]" for label, index in term)
            for term in polynomial
        )
        for polynomial in system
    )


def output_data_file_path(degree: int) -> str:
    return f"data/WeaklyCanonizedEquations-{degree:04}.txt"


def main():
    max_degree = 0
    while data_files_available(max_degree):
        max_degree += 1
    max_degree -= 1
    print(f"Data files of degree <= {max_degree} are available.", file=stderr)

    canonized_systems: dict[System, int] = {}
    for degree in range(max_degree + 1):
        print(f"Processing data files of degree {degree}.", file=stderr)
        with open(output_data_file_path(degree), "w") as file:
            for block in equation_block_iterator(degree):
                system = canonize(parse_system(block))
                if system in canonized_systems:
                    canonized_systems[system] += 1
                else:
                    canonized_systems[system] = 1
                    file.write(wolfram_string(system))
                    file.write("\n\n")


if __name__ == "__main__":
    main()
