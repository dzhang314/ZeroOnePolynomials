#!/usr/bin/env julia

using Base.Threads: @threads
using NautyGraphs: NautyGraph, add_edge!, canonize!, edges
using Printf: @sprintf

push!(LOAD_PATH, @__DIR__)
using EquationParser: load_pq_systems, print_canonical_system


function canonize(system::Vector{Vector{Tuple{Int,Int}}})
    @assert all(allunique.(system))
    ps = unique(p for equation in system for (p, q) in equation if !iszero(p))
    qs = unique(q for equation in system for (p, q) in equation if !iszero(q))
    terms = unique!(reduce(vcat, system))
    num_terms = length(terms)
    p_offset = 0
    p_map = Dict(p => i + p_offset for (i, p) in enumerate(ps))
    q_offset = p_offset + length(ps)
    q_map = Dict(q => i + q_offset for (i, q) in enumerate(qs))
    term_offset = q_offset + length(qs)
    term_map = Dict(t => i + term_offset for (i, t) in enumerate(terms))
    g = NautyGraph(vertex_labels=vcat(
        fill(0, term_offset), fill(1, num_terms), fill(2, length(system))))
    for term in terms
        (p, q) = term
        if !iszero(p)
            add_edge!(g, p_map[p], term_map[term])
        end
        if !iszero(q)
            add_edge!(g, q_map[q], term_map[term])
        end
    end
    equation_offset = term_offset + num_terms
    for (i, equation) in enumerate(system)
        for term in equation
            add_edge!(g, term_map[term], i + equation_offset)
        end
    end
    canonize!(g)
    canonical_terms = fill((0, 0), num_terms)
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


degree_pairs(d::Int) =
    [(m, d - m) for m in 5:((d-1)>>1) if (m == 5) || (m >= 7)]

data_file_path(d::Int, m::Int, n::Int) = joinpath("data",
    @sprintf("ZeroOneEquations-%04d-%04d-%04d.txt", d, m, n))

canonical_file_path(d::Int, m::Int, n::Int) = joinpath("data",
    @sprintf("CanonicalEquations-%04d-%04d-%04d.txt", d, m, n))


function main()
    canonical_systems = Set{Vector{Vector{Tuple{Int,Int}}}}()
    for d = 0:typemax(Int)
        for (m, n) in degree_pairs(d)
            input_path = data_file_path(d, m, n)
            if !isfile(input_path)
                println("File ", input_path, " not found.")
                flush(stdout)
                return nothing
            end
            count = 0
            println("Loading input file: ", input_path)
            flush(stdout)
            raw_systems = load_pq_systems(input_path)
            println("Loaded ", length(raw_systems), " systems.")
            flush(stdout)
            systems = similar(raw_systems)
            @threads for i in eachindex(raw_systems)
                systems[i] = canonize(raw_systems[i])
            end
            output_path = canonical_file_path(d, m, n)
            println("Writing output file: ", output_path)
            flush(stdout)
            open(output_path, "w") do io
                for system in systems
                    if !(system in canonical_systems)
                        push!(canonical_systems, system)
                        count += 1
                        print_canonical_system(io, system)
                        print(io, '\n')
                    end
                end
            end
            println("Wrote $count new canonical systems.")
            flush(stdout)
        end
        println("Finished processing degree $d.")
        flush(stdout)
    end
    return nothing
end


if abspath(PROGRAM_FILE) == @__FILE__
    main()
end
