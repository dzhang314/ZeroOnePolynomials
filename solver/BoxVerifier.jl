using FLINT_jll: libflint
using HiGHS: HighsInt, Highs_create, Highs_destroy,
    Highs_getModelStatus, Highs_getSolution, Highs_passLp,
    Highs_run, Highs_setBoolOptionValue, Highs_setDoubleOptionValue,
    kHighsMatrixFormatColwise, kHighsModelStatusOptimal,
    kHighsObjSenseMinimize, kHighsStatusOk
using SparseArrays: SparseMatrixCSC, SparseVector

################################################################################

@inline sort_tuple(i::Int, j::Int) = minmax(i, j)

@inline function sort_tuple(i::Int, j::Int, k::Int)
    i, k = minmax(i, k)
    i, j = minmax(i, j)
    j, k = minmax(j, k)
    return (i, j, k)
end

################################################################################

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

################################################################################

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

################################################################################

function build_terms(terms::Dict{NTuple{N,Int},Int}, i::Int, n::Int) where {N}
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

function construct_quadratic_box_constraints(n::Int)
    num_columns = div((2 * n + 1) * (2 * n), 2)
    num_entries = div((3 * n + 1) * (3 * n), 2)
    a_start = HighsInt[]
    a_index = HighsInt[]
    a_value = Cdouble[]
    sizehint!(a_start, num_columns + 1)
    sizehint!(a_index, num_entries)
    sizehint!(a_value, num_entries)
    push!(a_start, zero(HighsInt))
    terms = Dict{Tuple{Int,Int},Int}()
    terms[(0, 0)] = 1
    for i = 1:2*n
        terms_i = build_terms(terms, i, n)
        for j = i:2*n
            terms_j = build_terms(terms_i, j, n)
            for ((x, y), c) in terms_j
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

function construct_cubic_box_constraints(n::Int)
    num_columns = div((2 * n + 2) * (2 * n + 1) * (2 * n), 6)
    num_entries = div((3 * n + 2) * (3 * n + 1) * (3 * n), 6)
    a_start = HighsInt[]
    a_index = HighsInt[]
    a_value = Cdouble[]
    sizehint!(a_start, num_columns + 1)
    sizehint!(a_index, num_entries)
    sizehint!(a_value, num_entries)
    push!(a_start, zero(HighsInt))
    terms = Dict{Tuple{Int,Int,Int},Int}()
    terms[(0, 0, 0)] = 1
    for i = 1:2*n
        terms_i = build_terms(terms, i, n)
        for j = i:2*n
            terms_j = build_terms(terms_i, j, n)
            for k = j:2*n
                terms_k = build_terms(terms_j, k, n)
                for ((x, y, z), c) in terms_k
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

################################################################################

const Equation = Vector{Tuple{Int,Int}}
const System = Vector{Equation}

function add_linear_equation_constraint!(
    a_start::Vector{HighsInt},
    a_index::Vector{HighsInt},
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

function add_quadratic_equation_constraint!(
    a_start::Vector{HighsInt},
    a_index::Vector{HighsInt},
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

################################################################################

const QUADRATIC_LOCK = ReentrantLock()

const QUADRATIC_CACHE =
    Dict{Int,Tuple{Vector{HighsInt},Vector{HighsInt},Vector{Cdouble}}}()

const CUBIC_LOCK = ReentrantLock()

const CUBIC_CACHE =
    Dict{Int,Tuple{Vector{HighsInt},Vector{HighsInt},Vector{Cdouble}}}()

function construct_quadratic_constraint_matrix(system::System)
    n = maximum(max(i, j) for equation in system for (i, j) in equation)
    cache_result = lock(QUADRATIC_LOCK) do
        return get!(QUADRATIC_CACHE, n) do
            return construct_quadratic_box_constraints(n)
        end
    end
    a_start, a_index, a_value = copy.(cache_result)
    for equation in system
        if all(iszero(j) for (i, j) in equation)
            add_linear_equation_constraint!(
                a_start, a_index, a_value, equation, n)
            for j = 1:n
                add_linear_equation_constraint!(
                    a_start, a_index, a_value, equation, n, j)
            end
        else
            add_quadratic_equation_constraint!(
                a_start, a_index, a_value, equation, n)
        end
    end
    return (a_start, a_index, a_value)
end

function construct_cubic_constraint_matrix(system::System)
    n = maximum(max(i, j) for equation in system for (i, j) in equation)
    cache_result = lock(CUBIC_LOCK) do
        return get!(CUBIC_CACHE, n) do
            return construct_cubic_box_constraints(n)
        end
    end
    a_start, a_index, a_value = copy.(cache_result)
    for equation in system
        if all(iszero(j) for (i, j) in equation)
            add_linear_equation_constraint!(
                a_start, a_index, a_value, equation, n)
            for j = 1:n
                add_linear_equation_constraint!(
                    a_start, a_index, a_value, equation, n, j)
            end
            for j = 1:n
                for k = j:n
                    add_linear_equation_constraint!(
                        a_start, a_index, a_value, equation, n, j, k)
                end
            end
        else
            add_quadratic_equation_constraint!(
                a_start, a_index, a_value, equation, n)
            for k = 1:n
                add_quadratic_equation_constraint!(
                    a_start, a_index, a_value, equation, n, k)
            end
        end
    end
    return (a_start, a_index, a_value)
end

################################################################################

struct LinearProgram
    a_start::Vector{HighsInt}
    a_index::Vector{HighsInt}
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

function solve_linear_program(lp::LinearProgram)
    num_columns = length(lp.a_start) - 1
    num_entries = length(lp.a_index)
    num_rows = length(lp.b)
    column_cost = ones(Cdouble, num_columns)
    column_lower_bound = zeros(Cdouble, num_columns)
    column_upper_bound = fill(Cdouble(+Inf), num_columns)
    instance = Highs_create()
    try
        status = Highs_setBoolOptionValue(
            instance, "output_flag", zero(HighsInt))
        @assert status == kHighsStatusOk
        status = Highs_setDoubleOptionValue(
            instance, "kkt_tolerance", Cdouble(1.0e-10))
        @assert status == kHighsStatusOk
        status = Highs_passLp(instance,
            num_columns, num_rows, num_entries, kHighsMatrixFormatColwise,
            kHighsObjSenseMinimize, zero(Cdouble), column_cost,
            column_lower_bound, column_upper_bound, lp.b, lp.b,
            lp.a_start, lp.a_index, lp.a_value)
        @assert status == kHighsStatusOk
        status = Highs_run(instance)
        @assert status == kHighsStatusOk
        if Highs_getModelStatus(instance) != kHighsModelStatusOptimal
            return nothing
        end
        solution = Vector{Cdouble}(undef, num_columns)
        status = Highs_getSolution(instance, solution, C_NULL, C_NULL, C_NULL)
        @assert status == kHighsStatusOk
        return solution
    finally
        Highs_destroy(instance)
    end
end

################################################################################

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

################################################################################

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

################################################################################

function construct_reduced_matrix(lp::LinearProgram, indices::Vector{Int})
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

function solve_certified(lp::LinearProgram)
    numerical_solution = solve_linear_program(lp)
    if isnothing(numerical_solution)
        return false
    end
    support_indices = findall(>(1.0e-10), numerical_solution)
    A = construct_reduced_matrix(lp, support_indices)
    B = FlintIntegerMatrix(length(lp.b), 1)
    for i in eachindex(lp.b)
        if !iszero(lp.b[i])
            B[i, 1] = Int(lp.b[i])
        end
    end
    X = FlintRationalMatrix(length(support_indices), 1)
    ccall((:fmpq_mat_can_solve_fmpz_mat_multi_mod, libflint),
        Cint, (Ref{FlintRationalMatrix},
            Ref{FlintIntegerMatrix}, Ref{FlintIntegerMatrix}),
        X, A, B)
    a = SparseMatrixCSC(length(lp.b), length(lp.a_start) - 1,
        Int.(lp.a_start) .+ 1, Int.(lp.a_index) .+ 1, Int.(lp.a_value))
    x = SparseVector(length(lp.a_start) - 1,
        support_indices, [X[i, 1] for i = 1:length(support_indices)])
    b = a * x
    return (b.nzind == [1]) && (b.nzval == [1])
end
