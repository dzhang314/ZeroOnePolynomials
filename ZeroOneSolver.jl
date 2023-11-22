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


function main()
    println("Solving with ", nthreads(), " threads.")
    for degree = 1:typemax(Int)
        println(degree)
        for m = 1:((degree-1)>>1)
            n = degree - m
            for system in leaf_systems(UInt8, m, n)
                # println(system)
            end
        end
    end
end


main()
