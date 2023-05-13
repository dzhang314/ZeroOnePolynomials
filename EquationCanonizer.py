#!/usr/bin/env python3

import itertools
import os


def pair_iterator(k: int):
    for j in range(k + 1):
        i = k - j
        if i > j > 0:
            yield (i, j)


def equation_system_iterator(filename: str):
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


def reduced_equation_system_iterator(k: int):
    return itertools.chain(
        *[
            equation_system_iterator(f"data/ReducedEquations_{i:04}_{j:04}.txt")
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


def canonize_variables(system: System) -> System:
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


SUBSCRIPT_TABLE = str.maketrans(
    "0123456789", "\u2080\u2081\u2082\u2083\u2084\u2085\u2086\u2087\u2088\u2089"
)


def pretty_format(system: System) -> str:
    return (
        "{"
        + ", ".join(
            " + ".join(
                "*".join(
                    prefix + str(index).translate(SUBSCRIPT_TABLE)
                    for prefix, index in term
                )
                for term in equation
            )
            for equation in system
        )
        + "}"
    )


def canonized_fixed_point(system: System):
    system = sort_equation_system(canonize_variables(sort_equation_system(system)))
    while True:
        next_system = sort_equation_system(canonize_variables(system))
        if next_system == system:
            return system
        else:
            system = next_system


def main():
    seen = {}
    for k in itertools.count():
        for i, j in pair_iterator(k):
            if not os.path.isfile(f"data/ReducedEquations_{i:04}_{j:04}.txt"):
                print("Files for degree", k, "are not yet computed.")
                return
        file_name = f"data/CanonizedEquations_{k:04}.txt"
        temp_name = file_name + ".temp"
        with open(temp_name, "w+") as f:
            for system in reduced_equation_system_iterator(k):
                system = canonized_fixed_point(system_from_strs(system))
                if system not in seen:
                    seen[system] = True
                    f.write(pretty_format(system) + "\n")
        os.rename(temp_name, file_name)
        print(
            "Computed canonized equations of degree",
            f"{k} and saved to file {file_name}.",
            flush=True,
        )


if __name__ == "__main__":
    main()
