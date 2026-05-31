#!/usr/bin/env julia

using AbstractAlgebra: QQ, polynomial_ring
using Groebner: DegRevLex, groebner_with_change_matrix
using NautyGraphs: NautyGraph, add_edge!, canonize!, edges
using Printf: @sprintf


function degree_pairs(d::Int)
    return [(m, d - m) for m in 5:((d-1)>>1) if (m == 5) || (m >= 7)]
end


function data_file_path(m::Int, n::Int)
    return @sprintf("data/ZeroOneEquations-%04d-%04d-%04d.txt", m + n, m, n)
end


function data_files_available(d::Int)
    return all(isfile(data_file_path(m, n)) for (m, n) in degree_pairs(d))
end


function load_systems(path::AbstractString)
    data = read(path, String)
    result = Vector{Vector{Tuple{Int,Int}}}[]
    for system_str in strip.(split(data, "\n\n"; keepempty=false))
        system = Vector{Tuple{Int,Int}}[]
        for equation_str in strip.(split(system_str, '\n'))
            equation = Tuple{Int,Int}[]
            for term_str in strip.(split(equation_str, '+'))
                if '*' in term_str
                    factors = split(term_str, '*')
                    @assert length(factors) == 2
                    @assert startswith(factors[1], 'p')
                    @assert startswith(factors[2], 'q')
                    push!(equation, (
                        parse(Int, factors[1][2:end]),
                        parse(Int, factors[2][2:end])))
                elseif startswith(term_str, 'p')
                    push!(equation, (parse(Int, term_str[2:end]), 0))
                elseif startswith(term_str, 'q')
                    push!(equation, (0, parse(Int, term_str[2:end])))
                else
                    @assert false
                end
            end
            push!(system, equation)
        end
        push!(result, system)
    end
    return result
end


function canonize(system::Vector{Vector{Tuple{Int,Int}}})
    ps = unique(p for equation in system for (p, q) in equation if !iszero(p))
    qs = unique(q for equation in system for (p, q) in equation if !iszero(q))
    terms = unique!(reduce(vcat, system))
    vertex_labels = vcat(
        fill(0, length(ps)),
        fill(1, length(qs)),
        fill(2, length(terms)),
        fill(3, length(system)))
    p_offset = 0
    q_offset = p_offset + length(ps)
    term_offset = q_offset + length(qs)
    equation_offset = term_offset + length(terms)
    p_map = Dict(p => i + p_offset for (i, p) in enumerate(ps))
    q_map = Dict(q => i + q_offset for (i, q) in enumerate(qs))
    term_map = Dict(t => i + term_offset for (i, t) in enumerate(terms))
    g = NautyGraph(; vertex_labels)
    for term in terms
        (p, q) = term
        if !iszero(p)
            add_edge!(g, p_map[p], term_map[term])
        end
        if !iszero(q)
            add_edge!(g, q_map[q], term_map[term])
        end
    end
    for (i, equation) in enumerate(system)
        for term in equation
            add_edge!(g, term_map[term], i + equation_offset)
        end
    end
    canonize!(g)
    canonical_terms = fill((0, 0), length(terms))
    terms_frozen = false
    canonical_system = [Tuple{Int,Int}[] for _ in system]
    for e in edges(g)
        @assert e.src < e.dst
        if e.src <= q_offset
            @assert !terms_frozen
            p = e.src - p_offset
            i = e.dst - term_offset
            canonical_terms[i] = Base.setindex(canonical_terms[i], p, 1)
        elseif e.src <= term_offset
            @assert !terms_frozen
            q = e.src - q_offset
            i = e.dst - term_offset
            canonical_terms[i] = Base.setindex(canonical_terms[i], q, 2)
        else
            if !terms_frozen
                @assert allunique(canonical_terms)
                terms_frozen = true
            end
            i = e.src - term_offset
            j = e.dst - equation_offset
            push!(canonical_system[j], canonical_terms[i])
        end
    end
    return canonical_system
end


function has_no_solution(system::Vector{Vector{Tuple{Int,Int}}})
    p_max = maximum(p for equation in system for (p, q) in equation)
    q_max = maximum(q for equation in system for (p, q) in equation)
    names = vcat(Symbol.('p', 1:p_max), Symbol.('q', 1:q_max))
    _, vars = polynomial_ring(QQ, names)
    polynomials = map(system) do equation
        polynomial = -1
        for (p, q) in equation
            if iszero(p)
                @assert !iszero(q)
                polynomial += vars[q+p_max]
            elseif iszero(q)
                @assert !iszero(p)
                polynomial += vars[p]
            else
                polynomial += vars[p] * vars[q+p_max]
            end
        end
        return polynomial
    end
    _, matrix = groebner_with_change_matrix(polynomials;
        certify=true, linalg=:deterministic, ordering=DegRevLex())
    return matrix * polynomials == [1]
end


function main()
    d_max = 0
    while data_files_available(d_max)
        d_max += 1
    end
    d_max -= 1
    canonical_systems = Set{Vector{Vector{Tuple{Int,Int}}}}()
    for d = 0:d_max
        for (m, n) in degree_pairs(d)
            for system in load_systems(data_file_path(m, n))
                canonical_system = canonize(system)
                if !(canonical_system in canonical_systems)
                    if !has_no_solution(canonical_system)
                        println(canonical_system)
                        println(system)
                        println("ERROR: SATISFIABLE SYSTEM FOUND!")
                    end
                    push!(canonical_systems, canonical_system)
                end
            end
        end
        println("Degree $d: verified $(length(canonical_systems)) systems")
    end
    return nothing
end


if abspath(PROGRAM_FILE) == @__FILE__
    main()
end
