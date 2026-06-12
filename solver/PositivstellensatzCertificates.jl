module PositivstellensatzCertificates

using Clp_jll: libClp
using FLINT_jll: libflint
using HiGHS_jll: libhighs
using SparseArrays: SparseMatrixCSC, SparseVector

####################################################### EQUATION DATA STRUCTURES


const Term = Tuple{Int,Int}
const Equation = Vector{Term}
const System = Vector{Equation}


p_indices(system::System) = collect(BitSet(
    p for equation in system for (p, q) in equation if !iszero(p)))

q_indices(system::System) = collect(BitSet(
    q for equation in system for (p, q) in equation if !iszero(q)))


############################################################## LP DATA STRUCTURE


export LinearProgram


struct LinearProgram
    a_start::Vector{Cint}
    a_index::Vector{Cint}
    a_value::Vector{Cdouble}
    b::Vector{Cdouble}
end


################################################################ LP CONSTRUCTION


export construct_linear_program


function construct_monomials(n::Int, d::Int)
    result = [Int[]]
    i = 1
    while i <= length(result)
        monomial = result[i]
        if length(monomial) < d
            k = isempty(monomial) ? 1 : monomial[end]
            for j = k:n
                push!(result, push!(copy(monomial), j))
            end
        end
        i += 1
    end
    return result
end


insert_sorted(v::AbstractVector{T}, x::T) where {T} =
    insert!(copy(v), searchsortedlast(v, x) + 1, x)


function construct_box_product(indices::Vector{Int}, n::Int)
    result = Dict{Vector{Int},Int}(Int[] => 1)
    for i in indices
        next = Dict{Vector{Int},Int}()
        for (m, c) in result
            if i > n
                next[m] = get(next, m, 0) + c
                mi = insert_sorted(m, i - n)
                next[mi] = get(next, mi, 0) - c
            else
                mi = insert_sorted(m, i)
                next[mi] = get(next, mi, 0) + c
            end
        end
        result = next
    end
    return result
end


function construct_constraint_matrix(system::System, d::Int)
    ps = p_indices(system)
    qs = q_indices(system)
    np = length(ps)
    nq = length(qs)
    all_monomials = construct_monomials(np, d + 1)
    row_index = Dict{Vector{Int},Int}(
        m => (nq + 1) * (i - 1) for (i, m) in enumerate(all_monomials))
    num_rows = (nq + 1) * length(all_monomials)
    a_start = Cint[]
    a_index = Cint[]
    a_value = Cdouble[]
    push!(a_start, length(a_index))
    for indices in construct_monomials(2 * np, d + 1)
        box_product = construct_box_product(indices, np)
        if !isempty(indices)
            for (m, c) in box_product
                push!(a_index, row_index[m])
                push!(a_value, -c)
            end
            push!(a_start, length(a_index))
        end
        if length(indices) <= d
            for j = 1:nq
                for (m, c) in box_product
                    push!(a_index, row_index[m] + j)
                    push!(a_value, -c)
                end
                push!(a_start, length(a_index))
                for (m, c) in box_product
                    push!(a_index, row_index[m])
                    push!(a_value, -c)
                    push!(a_index, row_index[m] + j)
                    push!(a_value, +c)
                end
                push!(a_start, length(a_index))
            end
        end
    end
    p_map = Dict{Int,Int}(p => i for (i, p) in enumerate(ps))
    q_map = Dict{Int,Int}(q => i for (i, q) in enumerate(qs))
    _one = one(Cdouble)
    cofactor_monomials = construct_monomials(np, d)
    for equation in system
        for m in cofactor_monomials
            row_indices = Int[]
            for (p, q) in equation
                mp = iszero(p) ? m : insert_sorted(m, p_map[p])
                push!(row_indices, row_index[mp] + (iszero(q) ? 0 : q_map[q]))
            end
            push!(row_indices, row_index[m])
            append!(a_index, row_indices)
            append!(a_value, +_one for _ = 1:length(row_indices)-1)
            push!(a_value, -_one)
            push!(a_start, length(a_index))
            append!(a_index, row_indices)
            append!(a_value, -_one for _ = 1:length(row_indices)-1)
            push!(a_value, +_one)
            push!(a_start, length(a_index))
        end
    end
    return (num_rows, a_start, a_index, a_value)
end


function construct_linear_program(system::System, d::Int)
    num_rows, a_start, a_index, a_value =
        construct_constraint_matrix(system, d)
    @assert iszero(a_start[begin])
    @assert a_start[end] == length(a_index)
    @assert issorted(a_start) && allunique(a_start)
    @assert all(0 <= row_index < num_rows for row_index in a_index)
    @assert length(a_index) == length(a_value)
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
        status = ccall((:Highs_setIntOptionValue, libhighs),
            Cint, (Ptr{Cvoid}, Cstring, Cint),
            instance, "threads", one(Cint))
        @assert iszero(status)
        status = ccall((:Highs_setDoubleOptionValue, libhighs),
            Cint, (Ptr{Cvoid}, Cstring, Cdouble),
            instance, "primal_feasibility_tolerance", Cdouble(1.0e-10))
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
    entries = Vector{Rational{BigInt}}(undef, length(indices))
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
            value = X[i, 1]
            if signbit(value)
                return nothing
            end
            entries[i] = value
        end
    finally
        finalize(A)
        finalize(B)
        finalize(X)
    end
    a = SparseMatrixCSC(length(lp.b), length(lp.a_start) - 1,
        Int.(lp.a_start) .+ 1, Int.(lp.a_index) .+ 1, Int.(lp.a_value))
    x = SparseVector(length(lp.a_start) - 1, indices, entries)
    b = a * x
    return (b.nzind == [1]) && (b.nzval == [1]) ? (indices, entries) : nothing
end


######################################################### CERTIFICATE EXTRACTION


export print_certificate


function print_rational(io::IO, x::Rational{BigInt})
    @assert !iszero(x)
    print(io, numerator(x))
    d = denominator(x)
    if !isone(d)
        print(io, '/')
        print(io, d)
    end
    return nothing
end


function print_box_term(
    io::IO,
    c::Rational{BigInt},
    indices::Vector{Int},
    ps::Vector{Int},
    q::Int=0,
)
    @assert !signbit(c)
    @assert !isempty(indices)
    if !isone(c)
        print_rational(io, c)
        print(io, '*')
    end
    n = length(ps)
    first_factor = true
    for i in indices
        if first_factor
            first_factor = false
        else
            print(io, '*')
        end
        if i > n
            print(io, "(1-p")
            print(io, ps[i-n])
            print(io, ')')
        else
            print(io, 'p')
            print(io, ps[i])
        end
    end
    if q < 0
        print(io, "*(1-q")
        print(io, -q)
        print(io, ')')
    elseif q > 0
        print(io, "*q")
        print(io, q)
    end
    return nothing
end


function push_paired_term!(
    terms::Vector{String},
    solution::Dict{Int,Rational{BigInt}},
    solution_index::Int,
    p_indices::Vector{Int},
)
    entry_pos = get(solution, solution_index + 0, nothing)
    entry_neg = get(solution, solution_index + 1, nothing)
    if isnothing(entry_pos) & isnothing(entry_neg)
        return terms
    end
    entry = isnothing(entry_neg) ? entry_pos :
            isnothing(entry_pos) ? -entry_neg :
            entry_pos - entry_neg
    @assert !iszero(entry)
    term_buffer = IOBuffer()
    if isempty(p_indices)
        print_rational(term_buffer, entry)
    elseif isone(entry)
        print(term_buffer, 'p')
    elseif isone(-entry)
        print(term_buffer, "-p")
    else
        print_rational(term_buffer, entry)
        print(term_buffer, "*p")
    end
    if !isempty(p_indices)
        for i = 1:length(p_indices)-1
            print(term_buffer, p_indices[i])
            print(term_buffer, "*p")
        end
        print(term_buffer, p_indices[end])
    end
    push!(terms, String(take!(term_buffer)))
    return terms
end


function print_polynomial(io::IO, terms::Vector{String})
    if isempty(terms)
        print(io, '0')
    else
        print(io, terms[1])
        for i = 2:length(terms)
            if startswith(terms[i], '-')
                print(io, " - ")
                print(io, terms[i][2:end])
            else
                print(io, " + ")
                print(io, terms[i])
            end
        end
    end
    return nothing
end


function print_certificate(
    io::IO,
    system::System,
    d::Int,
    indices::Vector{Int},
    entries::Vector{Rational{BigInt}},
)
    ps = p_indices(system)
    qs = q_indices(system)
    np = length(ps)
    nq = length(qs)
    solution = Dict{Int,Rational{BigInt}}(indices .=> entries)
    index = 1
    box_buffer = IOBuffer()
    for indices in construct_monomials(2 * np, d + 1)
        if !isempty(indices)
            entry = get(solution, index, nothing)
            if !isnothing(entry)
                print_box_term(box_buffer, entry, indices, ps)
                print(box_buffer, '\n')
            end
            index += 1
        end
        if length(indices) <= d
            for j = 1:nq
                entry = get(solution, index, nothing)
                if !isnothing(entry)
                    print_box_term(box_buffer, entry, indices, ps, +qs[j])
                    print(box_buffer, '\n')
                end
                index += 1
                entry = get(solution, index, nothing)
                if !isnothing(entry)
                    print_box_term(box_buffer, entry, indices, ps, -qs[j])
                    print(box_buffer, '\n')
                end
                index += 1
            end
        end
    end
    cofactor_monomials = construct_monomials(np, d)
    for _ in system
        cofactor = String[]
        for m in cofactor_monomials
            push_paired_term!(cofactor, solution, index, ps[m])
            index += 2
        end
        print_polynomial(io, cofactor)
        print(io, '\n')
    end
    @assert all(<(index), indices)
    print(io, '\n')
    write(io, take!(box_buffer))
    return nothing
end


################################################################################

end # module PositivstellensatzCertificates
