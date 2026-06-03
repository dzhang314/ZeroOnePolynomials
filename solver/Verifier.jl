#!/usr/bin/env julia

using AbstractAlgebra: QQ, polynomial_ring
using Base.Threads: @spawn, Atomic, atomic_add!, atomic_sub!, nthreads, threadid
using Groebner: groebner_with_change_matrix
using NautyGraphs: NautyGraph, add_edge!, canonize!, edges
using Printf: @printf, @sprintf
using SHA: sha256


degree_pairs(d::Int) =
    [(m, d - m) for m in 5:((d-1)>>1) if (m == 5) || (m >= 7)]

data_file_path(m::Int, n::Int) = joinpath("data",
    @sprintf("ZeroOneEquations-%04d-%04d-%04d.txt", m + n, m, n))

data_files_available(d::Int) =
    all(isfile(data_file_path(m, n)) for (m, n) in degree_pairs(d))

proof_file_path(key::AbstractString) = joinpath("proofs", key * ".txt")


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
    @assert all(allunique.(system))
    ps = unique(p for equation in system for (p, q) in equation if !iszero(p))
    qs = unique(q for equation in system for (p, q) in equation if !iszero(q))
    terms = unique!(reduce(vcat, system))
    p_map = Dict(p => i for (i, p) in enumerate(ps))
    q_map = Dict(q => i + length(ps) for (i, q) in enumerate(qs))
    term_offset = length(ps) + length(qs)
    term_map = Dict(t => i + term_offset for (i, t) in enumerate(terms))
    g = NautyGraph(vertex_labels=vcat(
        fill(0, term_offset), fill(1, length(terms)), fill(2, length(system))))
    for term in terms
        (p, q) = term
        if !iszero(p)
            add_edge!(g, p_map[p], term_map[term])
        end
        if !iszero(q)
            add_edge!(g, q_map[q], term_map[term])
        end
    end
    equation_offset = term_offset + length(terms)
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
        if e.src <= term_offset
            @assert !terms_frozen
            i = e.dst - term_offset
            canonical_terms[i] = Base.setindex(canonical_terms[i],
                e.src, iszero(canonical_terms[i][1]) ? 1 : 2)
        else
            if !terms_frozen
                @assert allunique(canonical_terms)
                terms_frozen = true
            end
            push!(canonical_system[e.dst-equation_offset],
                canonical_terms[e.src-term_offset])
        end
    end
    return canonical_system
end


function find_infeasibility_certificate(system::Vector{Vector{Tuple{Int,Int}}})
    num_vars = maximum(max(a, b) for equation in system for (a, b) in equation)
    names = Symbol.('x', 1:num_vars)
    _, vars = polynomial_ring(QQ, names; internal_ordering=:degrevlex)
    polynomials = map(system) do equation
        polynomial = -1
        for (a, b) in equation
            @assert !iszero(a)
            polynomial += iszero(b) ? vars[a] : vars[a] * vars[b]
        end
        return polynomial
    end
    start = time_ns()
    for seed = 1:10
        _, matrix = groebner_with_change_matrix(polynomials; seed)
        if matrix * polynomials == [1]
            return (reshape(matrix, :), time_ns() - start)
        end
    end
    return (nothing, time_ns() - start)
end


term_string(i::Int, j::Int) =
    iszero(j) ? @sprintf("x%d", i) : @sprintf("x%d*x%d", i, j)

equation_string(equation::Vector{Tuple{Int,Int}}) =
    join((term_string(i, j) for (i, j) in equation), " + ")

system_string(system::Vector{Vector{Tuple{Int,Int}}}) =
    join((equation_string(equation) for equation in system), '\n')


function solve(system::Vector{Vector{Tuple{Int,Int}}}, m::Int, n::Int)
    system_str = system_string(system)
    key = bytes2hex(sha256(system_str))
    proof_path = proof_file_path(key)
    if isfile(proof_path)
        open(proof_path) do io
            if !startswith(io, system_str * "\n\n")
                println(stderr, "ERROR: Hash collision detected: $key")
                flush(stderr)
            end
        end
    else
        mkpath(dirname(proof_path))
        temp_path = proof_path * ".temp"
        open(temp_path, "w") do io
            println(io, system_str)
        end
        certificate, ns = find_infeasibility_certificate(system)
        if isnothing(certificate)
            println(stderr, "ERROR: Failed to prove: $key")
            println(stderr, system_string(system))
            println(stderr)
            flush(stderr)
        else
            println("Thread $(threadid()) proved $key in $(ns / 1.0e6) ms.")
            open(temp_path, "a") do io
                println(io)
                for polynomial in certificate
                    println(io, replace(string(polynomial), "//" => '/'))
                end
                println(io)
                @printf(io, "# computed in %.6f seconds\n", ns / 1.0e9)
                @printf(io, "# found at degree %d (%d, %d)\n", m + n, m, n)
            end
            mv(temp_path, proof_path)
        end
    end
    return nothing
end


function main()
    d_max = 0
    while data_files_available(d_max)
        d_max += 1
    end
    d_max -= 1
    println("Found data files up to degree $d_max.")
    flush(stdout)
    counters = Dict(d => Atomic{Int}(1) for d in 0:d_max)
    work = Channel{Tuple{Vector{Vector{Tuple{Int,Int}}},Int,Int}}(
        4 * nthreads())
    producer = @spawn begin
        try
            canonical_systems = Set{Vector{Vector{Tuple{Int,Int}}}}()
            for d = 0:d_max
                for (m, n) in degree_pairs(d)
                    for raw_system in load_systems(data_file_path(m, n))
                        system = canonize(raw_system)
                        if !(system in canonical_systems)
                            push!(canonical_systems, system)
                            atomic_add!(counters[d], 1)
                            put!(work, (system, m, n))
                        end
                    end
                end
                if isone(atomic_sub!(counters[d], 1))
                    println("Finished verification for degree $d.")
                    flush(stdout)
                end
            end
        finally
            close(work)
        end
    end
    consumers = map(1:nthreads()) do _
        @spawn for (system, m, n) in work
            try
                solve(system, m, n)
            catch err
                println(stderr, "ERROR: ", err)
                flush(stderr)
            finally
                if isone(atomic_sub!(counters[m+n], 1))
                    println("Finished verification for degree $(m + n).")
                    flush(stdout)
                end
            end
        end
    end
    foreach(wait, consumers)
    wait(producer)
    return nothing
end


if abspath(PROGRAM_FILE) == @__FILE__
    println("Starting up...")
    flush(stdout)
    find_infeasibility_certificate([
        [(2, 6), (5, 7)],
        [(4, 7), (5, 6)],
        [(3, 0), (2, 7)],
        [(2, 0), (4, 6)],
        [(4, 0), (3, 6)],
        [(5, 0), (1, 7)],
        [(3, 0), (1, 0)],
        [(2, 0), (7, 0)],
        [(1, 0), (6, 0)],
        [(7, 0), (6, 0)],
        [(4, 0), (7, 0), (1, 6)],
        [(5, 0), (6, 0), (3, 7)],
    ]) # warm up Groebner basis solver
    println("Running parallel verification with $(nthreads()) threads.")
    flush(stdout)
    main()
end
