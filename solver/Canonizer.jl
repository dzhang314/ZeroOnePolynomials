#!/usr/bin/env julia

using Base.Threads: @threads
using NautyGraphs: NautyGraph, add_edge!, canonize!, edges
using Printf: @sprintf


const _ZERO = UInt8('0')
const _NINE = UInt8('9')

@inline function parse_int(bytes::Vector{UInt8}, i::Int)
    n = length(bytes)
    result = 0
    @inbounds while i <= n
        b = bytes[i]
        if !((b >= _ZERO) & (b <= _NINE))
            break
        end
        result = muladd(10, result, Int(b - _ZERO))
        i += 1
    end
    return (result, i)
end


const _P = UInt8('p')
const _Q = UInt8('q')
const _STAR = UInt8('*')

@inline function parse_term(bytes::Vector{UInt8}, i::Int)
    n = length(bytes)
    @inbounds begin
        head = bytes[i]
        if head == _P
            p, i = parse_int(bytes, i + 1)
            if (i < n) && (bytes[i] == _STAR)
                @assert bytes[i+1] == _Q
                q, i = parse_int(bytes, i + 2)
                return ((p, q), i)
            else
                return ((p, 0), i)
            end
        else
            @assert head == _Q
            q, i = parse_int(bytes, i + 1)
            return ((0, q), i)
        end
    end
end


const _PLUS = UInt8('+')
const _SP = UInt8(' ')
const _NL = UInt8('\n')

function load_systems(path::AbstractString)
    bytes = read(path)
    n = length(bytes)
    equation = Tuple{Int,Int}[]
    system = Vector{Tuple{Int,Int}}[]
    result = Vector{Vector{Tuple{Int,Int}}}[]
    i = 1
    @inbounds while i <= n
        b = bytes[i]
        if (b == _PLUS) | (b == _SP)
            i += 1
        elseif b == _NL
            if isempty(equation)
                push!(result, system)
                system = Vector{Tuple{Int,Int}}[]
                i += 1
            else
                push!(system, equation)
                equation = Tuple{Int,Int}[]
                i += 1
            end
        else
            term, i = parse_term(bytes, i)
            push!(equation, term)
        end
    end
    @assert isempty(equation)
    @assert isempty(system)
    return result
end


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

term_string(i::Int, j::Int) =
    iszero(j) ? @sprintf("x%d", i) : @sprintf("x%d*x%d", i, j)

equation_string(equation::Vector{Tuple{Int,Int}}) =
    join((term_string(i, j) for (i, j) in equation), " + ")

system_string(system::Vector{Vector{Tuple{Int,Int}}}) =
    join((equation_string(equation) for equation in system), '\n')


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
            raw_systems = load_systems(input_path)
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
                        println(io, system_string(system), '\n')
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
