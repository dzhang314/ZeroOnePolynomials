using Base.Threads

push!(LOAD_PATH, @__DIR__)
using ZeroOnePolynomials


function leaf_systems(::Type{T}, m::Int, n::Int) where {T<:Integer}
    @assert 0 < m < n
    tasks = Task[]
    for k = zero(UInt64):((one(UInt64)<<(m-1))-one(UInt64))
        push!(tasks, @spawn solve(initial_system(T(m), T(n), k)))
    end
    result = System{T}[]
    for task in tasks
        for leaf in fetch(task)
            push!(result, leaf)
        end
    end
    return result
end


function print_system(io::IO, system::System{T}) where {T<:Integer}
    used_p = [false for _ in system.ps]
    used_q = [false for _ in system.qs]
    @assert length(system.lhs) == length(system.rhs)
    for (polynomial, rhs) in zip(system.lhs, system.rhs)
        if !isempty(polynomial.terms)
            @assert rhs == ZeroOnePolynomials.RHS_ONE
            for term in polynomial.terms
                if !iszero(term.p_index)
                    used_p[term.p_index] = true
                end
                if !iszero(term.q_index)
                    used_q[term.q_index] = true
                end
            end
            println(io, polynomial)
        end
    end
    for (var, used) in zip(system.ps, used_p)
        if var == ZeroOnePolynomials.VAR_ZERO
            @assert !used
        elseif var == ZeroOnePolynomials.VAR_ONE
            @assert !used
        elseif var == ZeroOnePolynomials.VAR_UNKNOWN
            @assert used
        else
            @assert false
        end
    end
    for (var, used) in zip(system.qs, used_q)
        if var == ZeroOnePolynomials.VAR_ZERO
            @assert !used
        elseif var == ZeroOnePolynomials.VAR_ONE
            @assert !used
        elseif var == ZeroOnePolynomials.VAR_UNKNOWN
            @assert used
        else
            @assert false
        end
    end
    println(io)
    return nothing
end


function main()
    println("Solving with ", nthreads(), " threads.")
    for degree = 1:typemax(Int)
        for m = 1:((degree-1)>>1)
            n = degree - m
            filename = "data/ZeroOneEquations_$(lpad(m + n, 4, '0'))_$(lpad(m, 4, '0'))_$(lpad(n, 4, '0')).txt"
            tempname = filename * ".temp"
            open(tempname, "w+") do io
                for system in leaf_systems(UInt8, m, n)
                    print_system(io, system)
                end
            end
            mv(tempname, filename)
            println("Finished computing $(filename).")
        end
    end
end


main()
