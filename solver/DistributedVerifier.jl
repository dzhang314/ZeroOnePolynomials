#!/usr/bin/env julia

using Distributed: @everywhere, RemoteChannel,
    myid, nworkers, remotecall, workers

println(stdout, "Running distributed verification with $(nworkers()) workers.")
flush(stdout)

using Printf: @sprintf
using SHA: sha256

include(joinpath(@__DIR__, "EquationParser.jl"))
using .EquationParser: print_system, load_systems

@everywhere include_string(Main,
    $(read(joinpath(@__DIR__, "PositivstellensatzCertificates.jl"), String)),
    "PositivstellensatzCertificates.jl")
@everywhere using .PositivstellensatzCertificates:
    construct_linear_program, solve_highs, solve_exact, print_certificate


@everywhere const Equation = Vector{Tuple{Int,Int}}
@everywhere const System = Vector{Equation}


@everywhere function prove_infeasible(system::System)
    for d = 0:4
        lp = construct_linear_program(system, d)
        numerical_solution = solve_highs(lp)
        if !isnothing(numerical_solution)
            exact_solution = solve_exact(
                lp, numerical_solution, Cdouble(1.0e-10))
            if !isnothing(exact_solution)
                return (d, exact_solution...)
            end
        end
    end
    return nothing
end


const WorkItem = Tuple{String,System}
const WorkerResult = Tuple{Int,Symbol,String,Union{
    Nothing,String,Tuple{Int,Vector{Int},Vector{Rational{BigInt}}}}}


@everywhere function worker_loop!(
    queue::RemoteChannel{Channel{WorkItem}},
    results::RemoteChannel{Channel{WorkerResult}},
)
    for (key, system) in queue
        put!(results, (myid(), :start, key, nothing))
        try
            result = prove_infeasible(system)
            put!(results, (myid(), :done, key, result))
        catch e
            message = sprint(showerror, e, catch_backtrace())
            put!(results, (myid(), :error, key, message))
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


function proof_exists(key::String, str::String)
    proof_path = proof_file_path(key)
    if isfile(proof_path)
        open(proof_path) do io
            @assert startswith(io, str)
        end
        return true
    else
        return false
    end
end


@inline to_digest(h::Vector{UInt8}) = ntuple(i -> @inbounds(h[i]), Val{32}())


function producer_loop!(
    seen::Set{NTuple{32,UInt8}},
    pending::Dict{String,System},
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
            for system in load_systems(input_path)
                str = sprint(print_system, system)
                h = sha256(str)
                digest = to_digest(h)
                @assert !(digest in seen)
                push!(seen, digest)
                key = bytes2hex(h)
                if proof_exists(key, str)
                    println(stdout, "Proof $key already exists.")
                    flush(stdout)
                else
                    pending[key] = system
                    put!(queue, (key, system))
                end
            end
        end
    end
    return nothing
end


function write_stub(key::String, system::System)
    proof_path = proof_file_path(key)
    mkpath(dirname(proof_path))
    open(proof_path * ".temp", "w") do io
        print_system(io, system)
    end
    return nothing
end


function mark_failed(key::String)
    proof_path = proof_file_path(key)
    mv(proof_path * ".temp", proof_path * ".failed"; force=true)
    return nothing
end


function write_proof(
    key::String,
    system::System,
    d::Int,
    indices::Vector{Int},
    entries::Vector{Rational{BigInt}},
)
    proof_path = proof_file_path(key)
    temp_path = proof_path * ".temp"
    open(temp_path, "a") do io
        print(io, '\n')
        print_certificate(io, system, d, indices, entries)
    end
    mv(temp_path, proof_path)
    return nothing
end


function main()
    seen = Set{NTuple{32,UInt8}}()
    pending = Dict{String,System}()
    systems = Dict{String,System}()
    queue = RemoteChannel{Channel{WorkItem}}(
        () -> Channel{WorkItem}(4 * nworkers()))
    results = RemoteChannel{Channel{WorkerResult}}(
        () -> Channel{WorkerResult}(4 * nworkers()))
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
    for (id, status, key, result) in results
        if status == :start
            @assert isnothing(result)
            println(stdout, "Worker $id started $key.")
            flush(stdout)
            write_stub(key, systems[key])
        elseif status == :done
            if isnothing(result)
                println(stderr, "Worker $id failed to prove $key.")
                flush(stderr)
                mark_failed(key)
                delete!(pending, key)
            else
                println(stdout, "Worker $id proved $key.")
                flush(stdout)
                write_proof(key, systems[key], result...)
                delete!(pending, key)
            end
        elseif status == :error
            println(stderr, "Worker $id threw error while proving $key.")
            println(stderr, result)
            flush(stderr)
            mark_failed(key)
            delete!(pending, key)
        else
            @assert false
        end
    end
    wait(producer)
    wait(consumer)
    return nothing
end


main()
