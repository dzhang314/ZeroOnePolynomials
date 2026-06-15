#!/usr/bin/env julia

using Distributed: @everywhere, RemoteChannel,
    myid, nworkers, remotecall, workers

println(stdout, "Running distributed verification with $(nworkers()) workers.")
flush(stdout)

using Printf: @sprintf

@everywhere using SHA: sha256

@everywhere include_string(Main,
    $(read(joinpath(@__DIR__, "EquationParser.jl"), String)),
    "EquationParser.jl")
@everywhere using .EquationParser: parse_system

@everywhere include_string(Main,
    $(read(joinpath(@__DIR__, "PositivstellensatzCertificates.jl"), String)),
    "PositivstellensatzCertificates.jl")
@everywhere using .PositivstellensatzCertificates:
    construct_linear_program, solve_highs, solve_exact, print_certificate


@everywhere const Equation = Vector{Tuple{Int,Int}}
@everywhere const System = Vector{Equation}
@everywhere const WorkItem = Tuple{NTuple{32,UInt8},Vector{UInt8}}
@everywhere const WorkerResult = Tuple{
    Int,Symbol,NTuple{32,UInt8},Union{Nothing,String,Tuple{Int,String}}}


@everywhere function prove_infeasible(system::System)
    for d = 0:4
        lp = construct_linear_program(system, d)
        numerical_solution = solve_highs(lp)
        if !isnothing(numerical_solution)
            exact_solution = solve_exact(
                lp, numerical_solution, Cdouble(1.0e-10))
            if !isnothing(exact_solution)
                return (d, sprint(print_certificate,
                    system, d, exact_solution...))
            end
        end
    end
    return nothing
end


degree_pairs(d::Int) =
    [(m, d - m) for m in 5:((d-1)>>1) if (m == 5) || (m >= 7)]

data_file_path(d::Int, m::Int, n::Int) = joinpath("data",
    @sprintf("ZeroOneEquations-%04d-%04d-%04d.txt", d, m, n))

proof_file_path(key::String) =
    joinpath("proofs", key[1:2], key[3:4], key * ".txt")


function proof_exists(digest::NTuple{32,UInt8}, bytes::Vector{UInt8})
    proof_path = proof_file_path(bytes2hex(digest))
    if isfile(proof_path)
        open(proof_path) do io
            @assert read(io, length(bytes)) == bytes
        end
        return true
    else
        return false
    end
end


const _NEWLINE = UInt8('\n')
const _BLOCK_SIZE = 1 << 20
const _INPUT_CHANNEL_SIZE = 1 << 10

function load_systems(input_path::AbstractString)
    return Channel{Vector{UInt8}}(_INPUT_CHANNEL_SIZE) do channel
        open(input_path) do io
            block = Vector{UInt8}(undef, _BLOCK_SIZE)
            bytes = UInt8[]
            start = 1
            while !eof(io)
                num_read = readbytes!(io, block, _BLOCK_SIZE)
                append!(bytes, view(block, 1:num_read))
                n = length(bytes)
                i = start
                @inbounds while i < n
                    if (bytes[i] == _NEWLINE) & (bytes[i+1] == _NEWLINE)
                        put!(channel, bytes[start:i+1])
                        i += 2
                        start = i
                    else
                        i += 1
                    end
                end
                deleteat!(bytes, 1:start-1)
                start = 1
            end
            @assert isempty(bytes)
            return nothing
        end
        return nothing
    end
end


@everywhere @inline to_digest(h::Vector{UInt8}) =
    ntuple(i -> @inbounds(h[i]), Val{32}())


function producer_loop!(
    seen::Set{NTuple{32,UInt8}},
    pending::Dict{NTuple{32,UInt8},Vector{UInt8}},
    queue::RemoteChannel{Channel{WorkItem}},
)
    for d = 0:typemax(Int)
        for (m, n) in degree_pairs(d)
            input_path = data_file_path(d, m, n)
            if !isfile(input_path)
                println(stderr, "File ", input_path, " not found.")
                flush(stderr)
                return nothing
            end
            for bytes in load_systems(input_path)
                digest = to_digest(sha256(bytes))
                @assert !(digest in seen)
                push!(seen, digest)
                if !proof_exists(digest, bytes)
                    pending[digest] = bytes
                    put!(queue, (digest, bytes))
                end
            end
            println("Finished issuing work for: ", (d, m, n))
            flush(stdout)
        end
    end
    return nothing
end


@everywhere function worker_loop!(
    queue::RemoteChannel{Channel{WorkItem}},
    results::RemoteChannel{Channel{WorkerResult}},
)
    for (digest, bytes) in queue
        try
            put!(results, (myid(), :start, digest, nothing))
            @assert digest == to_digest(sha256(bytes))
            system, i = parse_system(bytes, firstindex(bytes))
            @assert i == lastindex(bytes) + 1
            result = prove_infeasible(system)
            put!(results, (myid(), :done, digest, result))
        catch e
            message = sprint(showerror, e, catch_backtrace())
            put!(results, (myid(), :error, digest, message))
        end
    end
    return nothing
end


function write_stub(key::String, bytes::Vector{UInt8})
    proof_path = proof_file_path(key)
    mkpath(dirname(proof_path))
    open(proof_path * ".temp", "w") do io
        write(io, bytes)
    end
    return nothing
end


function mark_failed(key::String)
    proof_path = proof_file_path(key)
    mv(proof_path * ".temp", proof_path * ".failed"; force=true)
    return nothing
end


function write_proof(key::String, proof::String)
    proof_path = proof_file_path(key)
    temp_path = proof_path * ".temp"
    open(temp_path, "a") do io
        print(io, proof)
    end
    mv(temp_path, proof_path)
    return nothing
end


function main()
    seen = Set{NTuple{32,UInt8}}()
    pending = Dict{NTuple{32,UInt8},Vector{UInt8}}()
    queue = RemoteChannel(() -> Channel{WorkItem}(4 * nworkers()))
    results = RemoteChannel(() -> Channel{WorkerResult}(4 * nworkers()))
    producer = @async try
        producer_loop!(seen, pending, queue)
    finally
        close(queue)
    end
    futures = map(id -> remotecall(worker_loop!, id, queue, results), workers())
    consumer = @async try
        foreach(wait, futures)
    finally
        close(results)
    end
    for (id, status, digest, result) in results
        key = bytes2hex(digest)
        if status == :start
            @assert isnothing(result)
            write_stub(key, pending[digest])
            delete!(pending, digest)
        elseif status == :done
            if isnothing(result)
                println(stderr, "Worker $id failed to prove $key.")
                flush(stderr)
                mark_failed(key)
            else
                d, proof = result
                println(stdout, "Worker $id proved $key at degree $d.")
                flush(stdout)
                write_proof(key, proof)
            end
        elseif status == :error
            println(stderr, "Worker $id threw error while proving $key.")
            println(stderr, result)
            flush(stderr)
            mark_failed(key)
        else
            @assert false
        end
    end
    wait(producer)
    wait(consumer)
    return nothing
end


main()
