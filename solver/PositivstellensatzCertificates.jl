module PositivstellensatzCertificates

using Clp_jll: libClp
using FLINT_jll: libflint
using HiGHS_jll: libhighs
using SparseArrays: SparseMatrixCSC, SparseVector

############################################################## MONOMIAL INDEXING


@inline sort_tuple(i::Int, j::Int) = minmax(i, j)

@inline function sort_tuple(i::Int, j::Int, k::Int)
    i, k = minmax(i, k)
    i, j = minmax(i, j)
    j, k = minmax(j, k)
    return (i, j, k)
end


@inline sorted_monomial_index(::Int) = 0

@inline sorted_monomial_index(::Int, i::Int) = i

@inline sorted_monomial_index(n::Int, i::Int, j::Int) =
    iszero(i) ? sorted_monomial_index(n, j) :
    n + div((2 * n - i + 2) * (i - 1), 2) + (j - i + 1)

@inline sorted_monomial_index(n::Int, i::Int, j::Int, k::Int) =
    iszero(i) ? sorted_monomial_index(n, j, k) :
    n + div((n + 1) * n, 2) +
    div((n + 2) * (n + 1) * n -
        (n - i + 3) * (n - i + 2) * (n - i + 1), 6) +
    div((2 * n - i - j + 3) * (j - i), 2) + (k - j + 1)


@inline monomial_index(n::Int) = sorted_monomial_index(n)

@inline monomial_index(n::Int, i::Int) = sorted_monomial_index(n, i)

@inline function monomial_index(n::Int, i::Int, j::Int)
    i, j = sort_tuple(i, j)
    return sorted_monomial_index(n, i, j)
end

@inline function monomial_index(n::Int, i::Int, j::Int, k::Int)
    i, j, k = sort_tuple(i, j, k)
    return sorted_monomial_index(n, i, j, k)
end


################################################################# BOX GENERATORS


function build_box_product(
    terms::Dict{NTuple{N,Int},Int},
    i::Int,
    n::Int,
) where {N}
    if i > n
        result = copy(terms)
        for ((z, j...), c) in terms
            @assert iszero(z)
            key = sort_tuple(i - n, j...)
            result[key] = get(result, key, 0) - c
        end
        return result
    else
        result = Dict{NTuple{N,Int},Int}()
        for ((z, j...), c) in terms
            @assert iszero(z)
            key = sort_tuple(i, j...)
            result[key] = c
        end
        return result
    end
end


function construct_quadratic_box_generators(n::Int)
    num_columns = div((2 * n + 1) * (2 * n), 2)
    num_entries = div((3 * n + 1) * (3 * n), 2)
    a_start = Cint[]
    a_index = Cint[]
    a_value = Cdouble[]
    sizehint!(a_start, num_columns + 1)
    sizehint!(a_index, num_entries)
    sizehint!(a_value, num_entries)
    push!(a_start, zero(Cint))
    box_product = Dict{Tuple{Int,Int},Int}()
    box_product[(0, 0)] = 1
    for i = 1:2*n
        box_product_i = build_box_product(box_product, i, n)
        for j = i:2*n
            box_product_ij = build_box_product(box_product_i, j, n)
            for ((x, y), c) in box_product_ij
                push!(a_index, sorted_monomial_index(n, x, y))
                push!(a_value, -c)
            end
            push!(a_start, length(a_index))
        end
    end
    @assert length(a_start) == num_columns + 1
    @assert length(a_index) == num_entries
    @assert length(a_value) == num_entries
    return (a_start, a_index, a_value)
end


function construct_cubic_box_generators(n::Int)
    num_columns = div((2 * n + 2) * (2 * n + 1) * (2 * n), 6)
    num_entries = div((3 * n + 2) * (3 * n + 1) * (3 * n), 6)
    a_start = Cint[]
    a_index = Cint[]
    a_value = Cdouble[]
    sizehint!(a_start, num_columns + 1)
    sizehint!(a_index, num_entries)
    sizehint!(a_value, num_entries)
    push!(a_start, zero(Cint))
    box_product = Dict{Tuple{Int,Int,Int},Int}()
    box_product[(0, 0, 0)] = 1
    for i = 1:2*n
        box_product_i = build_box_product(box_product, i, n)
        for j = i:2*n
            box_product_ij = build_box_product(box_product_i, j, n)
            for k = j:2*n
                box_product_ijk = build_box_product(box_product_ij, k, n)
                for ((x, y, z), c) in box_product_ijk
                    push!(a_index, sorted_monomial_index(n, x, y, z))
                    push!(a_value, -c)
                end
                push!(a_start, length(a_index))
            end
        end
    end
    @assert length(a_start) == num_columns + 1
    @assert length(a_index) == num_entries
    @assert length(a_value) == num_entries
    return (a_start, a_index, a_value)
end


############################################################### IDEAL GENERATORS


const Equation = Vector{Tuple{Int,Int}}
const System = Vector{Equation}


function add_ideal_generators_linear!(
    a_start::Vector{Cint},
    a_index::Vector{Cint},
    a_value::Vector{Cdouble},
    equation::Equation,
    n::Int,
    j::Int...,
)
    _one = one(Cdouble)
    for (i, _) in equation
        push!(a_index, monomial_index(n, i, j...))
        push!(a_value, +_one)
    end
    push!(a_index, monomial_index(n, j...))
    push!(a_value, -_one)
    push!(a_start, length(a_index))
    for (i, _) in equation
        push!(a_index, monomial_index(n, i, j...))
        push!(a_value, -_one)
    end
    push!(a_index, monomial_index(n, j...))
    push!(a_value, +_one)
    push!(a_start, length(a_index))
    return (a_start, a_index, a_value)
end


function add_ideal_generators_quadratic!(
    a_start::Vector{Cint},
    a_index::Vector{Cint},
    a_value::Vector{Cdouble},
    equation::Equation,
    n::Int,
    k::Int...,
)
    _one = one(Cdouble)
    for (i, j) in equation
        push!(a_index, monomial_index(n, i, j, k...))
        push!(a_value, +_one)
    end
    push!(a_index, monomial_index(n, k...))
    push!(a_value, -_one)
    push!(a_start, length(a_index))
    for (i, j) in equation
        push!(a_index, monomial_index(n, i, j, k...))
        push!(a_value, -_one)
    end
    push!(a_index, monomial_index(n, k...))
    push!(a_value, +_one)
    push!(a_start, length(a_index))
    return (a_start, a_index, a_value)
end


const QUADRATIC_LOCK = ReentrantLock()

const CUBIC_LOCK = ReentrantLock()

const QUADRATIC_CACHE =
    Dict{Int,Tuple{Vector{Cint},Vector{Cint},Vector{Cdouble}}}()

const CUBIC_CACHE =
    Dict{Int,Tuple{Vector{Cint},Vector{Cint},Vector{Cdouble}}}()


function construct_quadratic_constraint_matrix(system::System)
    n = maximum(max(i, j) for equation in system for (i, j) in equation)
    cache_result = lock(QUADRATIC_LOCK) do
        return get!(QUADRATIC_CACHE, n) do
            return construct_quadratic_box_generators(n)
        end
    end
    a_start, a_index, a_value = copy.(cache_result)
    for equation in system
        if all(iszero(j) for (i, j) in equation)
            add_ideal_generators_linear!(
                a_start, a_index, a_value, equation, n)
            for j = 1:n
                add_ideal_generators_linear!(
                    a_start, a_index, a_value, equation, n, j)
            end
        else
            add_ideal_generators_quadratic!(
                a_start, a_index, a_value, equation, n)
        end
    end
    return (a_start, a_index, a_value)
end


function construct_cubic_constraint_matrix(system::System)
    n = maximum(max(i, j) for equation in system for (i, j) in equation)
    cache_result = lock(CUBIC_LOCK) do
        return get!(CUBIC_CACHE, n) do
            return construct_cubic_box_generators(n)
        end
    end
    a_start, a_index, a_value = copy.(cache_result)
    for equation in system
        if all(iszero(j) for (i, j) in equation)
            add_ideal_generators_linear!(
                a_start, a_index, a_value, equation, n)
            for j = 1:n
                add_ideal_generators_linear!(
                    a_start, a_index, a_value, equation, n, j)
            end
            for j = 1:n
                for k = j:n
                    add_ideal_generators_linear!(
                        a_start, a_index, a_value, equation, n, j, k)
                end
            end
        else
            add_ideal_generators_quadratic!(
                a_start, a_index, a_value, equation, n)
            for k = 1:n
                add_ideal_generators_quadratic!(
                    a_start, a_index, a_value, equation, n, k)
            end
        end
    end
    return (a_start, a_index, a_value)
end


############################################################## LP DATA STRUCTURE


export LinearProgram, construct_linear_program


struct LinearProgram
    a_start::Vector{Cint}
    a_index::Vector{Cint}
    a_value::Vector{Cdouble}
    b::Vector{Cdouble}
end


function construct_linear_program(system::System, problem::Symbol)
    n = maximum(max(i, j) for equation in system for (i, j) in equation)
    num_rows = nothing
    constraint_matrix = nothing
    if problem == :quadratic
        num_rows = div((n + 2) * (n + 1), 2)
        constraint_matrix = construct_quadratic_constraint_matrix(system)
    elseif problem == :cubic
        num_rows = div((n + 3) * (n + 2) * (n + 1), 6)
        constraint_matrix = construct_cubic_constraint_matrix(system)
    else
        @assert false
    end
    a_start, a_index, a_value = constraint_matrix
    @assert iszero(a_start[begin])
    @assert a_start[end] == length(a_index)
    @assert issorted(a_start) && allunique(a_start)
    @assert length(a_index) == length(a_value)
    for row_index in a_index
        @assert 0 <= row_index < num_rows
    end
    b = zeros(Cdouble, num_rows)
    b[begin] = one(Cdouble)
    return LinearProgram(a_start, a_index, a_value, b)
end


############################################################ LP SOLVER INTERFACE


export solve_highs, solve_clp


function l0_cost(lp::LinearProgram)
    num_columns = length(lp.a_start) - 1
    result = Vector{Cdouble}(undef, num_columns)
    @simd ivdep for j = 1:num_columns
        @inbounds result[j] = lp.a_start[j+1] - lp.a_start[j]
    end
    return result
end


const HIGHS_MODEL_STATUS_OPTIMAL = Cint(7)


function solve_highs(lp::LinearProgram)
    num_columns = length(lp.a_start) - 1
    num_entries = length(lp.a_index)
    num_rows = length(lp.b)
    weights = l0_cost(lp)
    column_lower_bound = zeros(Cdouble, num_columns)
    column_upper_bound = fill(Cdouble(+Inf), num_columns)
    instance = ccall((:Highs_create, libhighs), Ptr{Cvoid}, ())
    try
        status = ccall((:Highs_setBoolOptionValue, libhighs),
            Cint, (Ptr{Cvoid}, Cstring, Cint),
            instance, "output_flag", zero(Cint))
        @assert iszero(status)
        status = ccall((:Highs_setDoubleOptionValue, libhighs),
            Cint, (Ptr{Cvoid}, Cstring, Cdouble),
            instance, "kkt_tolerance", Cdouble(1.0e-10))
        @assert iszero(status)
        status = ccall((:Highs_passLp, libhighs),
            Cint, (Ptr{Cvoid}, Cint, Cint, Cint, Cint, Cint, Cdouble,
                Ptr{Cdouble}, Ptr{Cdouble}, Ptr{Cdouble},
                Ptr{Cdouble}, Ptr{Cdouble},
                Ptr{Cint}, Ptr{Cint}, Ptr{Cdouble}),
            instance, num_columns, num_rows, num_entries,
            one(Cint), one(Cint), zero(Cdouble),
            weights, column_lower_bound, column_upper_bound, lp.b, lp.b,
            lp.a_start, lp.a_index, lp.a_value)
        @assert iszero(status)
        status = ccall((:Highs_run, libhighs), Cint, (Ptr{Cvoid},), instance)
        @assert iszero(status) | isone(status)
        model_status = ccall((:Highs_getModelStatus, libhighs),
            Cint, (Ptr{Cvoid},), instance)
        if model_status != HIGHS_MODEL_STATUS_OPTIMAL
            return nothing
        end
        solution = Vector{Cdouble}(undef, num_columns)
        status = ccall((:Highs_getSolution, libhighs),
            Cint, (Ptr{Cvoid},
                Ptr{Cdouble}, Ptr{Cdouble}, Ptr{Cdouble}, Ptr{Cdouble}),
            instance, solution, C_NULL, C_NULL, C_NULL)
        @assert iszero(status)
        return solution
    finally
        ccall((:Highs_destroy, libhighs), Cvoid, (Ptr{Cvoid},), instance)
    end
end


function solve_clp(lp::LinearProgram)
    num_columns = length(lp.a_start) - 1
    num_rows = length(lp.b)
    weights = l0_cost(lp)
    instance = ccall((:Clp_newModel, libClp), Ptr{Cvoid}, ())
    try
        ccall((:Clp_setLogLevel, libClp),
            Cvoid, (Ptr{Cvoid}, Cint), instance, zero(Cint))
        ccall((:Clp_loadProblem, libClp),
            Cvoid, (Ptr{Cvoid}, Cint, Cint,
                Ptr{Cint}, Ptr{Cint}, Ptr{Cdouble},
                Ptr{Cdouble}, Ptr{Cdouble}, Ptr{Cdouble},
                Ptr{Cdouble}, Ptr{Cdouble}),
            instance, num_columns, num_rows,
            lp.a_start, lp.a_index, lp.a_value,
            C_NULL, C_NULL, weights, lp.b, lp.b)
        status = ccall((:Clp_initialSolve, libClp),
            Cint, (Ptr{Cvoid},), instance)
        if !iszero(status)
            return nothing
        end
        ptr = ccall((:Clp_getColSolution, libClp),
            Ptr{Cdouble}, (Ptr{Cvoid},), instance)
        @assert ptr != C_NULL
        result = Vector{Cdouble}(undef, num_columns)
        unsafe_copyto!(pointer(result), ptr, num_columns)
        return result
    finally
        ccall((:Clp_deleteModel, libClp), Cvoid, (Ptr{Cvoid},), instance)
    end
end


################################################################# FLINT MATRICES


mutable struct FlintIntegerMatrix
    entries::Ptr{Int}
    r::Int
    c::Int
    stride::Int
    function FlintIntegerMatrix(num_rows::Int, num_columns::Int)
        A = new()
        ccall((:fmpz_mat_init, libflint),
            Cvoid, (Ref{FlintIntegerMatrix}, Int, Int),
            A, num_rows, num_columns)
        finalizer(A) do B
            ccall((:fmpz_mat_clear, libflint),
                Cvoid, (Ref{FlintIntegerMatrix},), B)
        end
        return A
    end
end


@inline function Base.setindex!(A::FlintIntegerMatrix, v::Int, i::Int, j::Int)
    ptr = A.entries + ((i - 1) * A.stride + (j - 1)) * sizeof(Int)
    ccall((:fmpz_set_si, libflint), Cvoid, (Ptr{Int}, Int), ptr, v)
    return A
end


struct FlintRational
    num::Int
    den::Int
end


mutable struct FlintRationalMatrix
    entries::Ptr{FlintRational}
    r::Int
    c::Int
    stride::Int
    function FlintRationalMatrix(num_rows::Int, num_columns::Int)
        A = new()
        ccall((:fmpq_mat_init, libflint),
            Cvoid, (Ref{FlintRationalMatrix}, Int, Int),
            A, num_rows, num_columns)
        finalizer(A) do B
            ccall((:fmpq_mat_clear, libflint),
                Cvoid, (Ref{FlintRationalMatrix},), B)
        end
        return A
    end
end


function Base.getindex(A::FlintRationalMatrix, i::Int, j::Int)
    ptr = A.entries + ((i - 1) * A.stride + (j - 1)) * sizeof(FlintRational)
    num = BigInt()
    ccall((:fmpz_get_mpz, libflint), Cvoid, (Ref{BigInt}, Ptr{Int}), num, ptr)
    ptr += sizeof(Int)
    den = BigInt()
    ccall((:fmpz_get_mpz, libflint), Cvoid, (Ref{BigInt}, Ptr{Int}), den, ptr)
    return Rational{BigInt}(num, den)
end


################################################################### EXACT SOLVER


export solve_exact


function construct_flint_submatrix(lp::LinearProgram, indices::Vector{Int})
    num_rows = length(lp.b)
    num_columns = length(lp.a_start) - 1
    A = FlintIntegerMatrix(num_rows, length(indices))
    for (j, column_index) in enumerate(indices)
        @assert 1 <= column_index <= num_columns
        for p = Int(lp.a_start[column_index])+1:Int(lp.a_start[column_index+1])
            A[Int(lp.a_index[p])+1, j] = Int(lp.a_value[p])
        end
    end
    return A
end


function solve_exact(
    lp::LinearProgram,
    numerical_solution::Vector{Cdouble},
    threshold::Cdouble,
)
    indices = findall(>(threshold), numerical_solution)
    values = Vector{Rational{BigInt}}(undef, length(indices))
    A = construct_flint_submatrix(lp, indices)
    B = FlintIntegerMatrix(length(lp.b), 1)
    X = FlintRationalMatrix(length(indices), 1)
    try
        for (i, v) in enumerate(lp.b)
            if !iszero(v)
                B[i, 1] = Int(v)
            end
        end
        status = ccall((:fmpq_mat_can_solve_fmpz_mat_multi_mod, libflint),
            Cint, (Ref{FlintRationalMatrix},
                Ref{FlintIntegerMatrix}, Ref{FlintIntegerMatrix}),
            X, A, B)
        if iszero(status)
            return nothing
        end
        for i = 1:length(indices)
            values[i] = X[i, 1]
        end
    finally
        finalize(A)
        finalize(B)
        finalize(X)
    end
    a = SparseMatrixCSC(length(lp.b), length(lp.a_start) - 1,
        Int.(lp.a_start) .+ 1, Int.(lp.a_index) .+ 1, Int.(lp.a_value))
    x = SparseVector(length(lp.a_start) - 1, indices, values)
    b = a * x
    return ((b.nzind == [1]) && (b.nzval == [1])) ? (indices, values) : nothing
end


################################################################################

end # module PositivstellensatzCertificates
