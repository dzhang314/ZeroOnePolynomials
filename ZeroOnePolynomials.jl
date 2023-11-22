module ZeroOnePolynomials


################################################################################


export RHS, RHS_ZERO, RHS_ONE, RHS_ZERO_OR_ONE
export VAR, VAR_ZERO, VAR_ONE, VAR_ZERO_OR_ONE, VAR_UNKNOWN


@enum RHS::UInt8 begin
    RHS_ZERO = 0
    RHS_ONE = 1
    RHS_ZERO_OR_ONE = 2
end


@enum VAR::UInt8 begin
    VAR_ZERO = 0
    VAR_ONE = 1
    VAR_ZERO_OR_ONE = 2
    VAR_UNKNOWN = 3
end


################################################################################


export Term, Polynomial, System


struct Term{T<:Integer}
    p_index::T
    q_index::T
end


function Base.show(io::IO, term::Term{T}) where {T<:Integer}
    if iszero(term.p_index)
        if iszero(term.q_index)
            print(io, '1')
        else
            print(io, 'q')
            print(io, term.q_index)
        end
    else
        print(io, 'p')
        print(io, term.p_index)
        if !iszero(term.q_index)
            print(io, '*')
            print(io, 'q')
            print(io, term.q_index)
        end
    end
    return nothing
end


struct Polynomial{T<:Integer}
    terms::Vector{Term{T}}
end


function Base.show(io::IO, polynomial::Polynomial{T}) where {T<:Integer}
    first = true
    for term in polynomial.terms
        if first
            first = false
        else
            print(io, " + ")
        end
        print(io, term)
    end
    return nothing
end


struct System{T<:Integer}
    ps::Vector{VAR}
    qs::Vector{VAR}
    lhs::Vector{Polynomial{T}}
    rhs::Vector{RHS}
end


function Base.show(io::IO, system::System{T}) where {T<:Integer}

    @assert length(system.lhs) == length(system.rhs)

    used_p = [false for _ in system.ps]
    used_q = [false for _ in system.qs]
    for lhs in system.lhs
        for term in lhs.terms
            if !iszero(term.p_index)
                used_p[term.p_index] = true
            end
            if !iszero(term.q_index)
                used_q[term.q_index] = true
            end
        end
    end

    lhs_strs = [string(poly) for poly in system.lhs]
    lhs_maxlen = maximum(length(str) for str in lhs_strs)

    for (i, (lhs_str, rhs)) in enumerate(zip(lhs_strs, system.rhs))
        print(io, lpad(i, 4))
        print(io, ": ")
        print(io, lpad(lhs_str, lhs_maxlen))
        if rhs == RHS_ZERO
            println(io, " == 0")
        elseif rhs == RHS_ONE
            println(io, " == 1")
        elseif rhs == RHS_ZERO_OR_ONE
            println(io, " == 0 or 1")
        end
    end

    for (i, (var, used)) in enumerate(zip(system.ps, used_p))
        print(io, lpad('p' * string(i), lhs_maxlen + 6))
        if var == VAR_ZERO
            @assert !used
            println(io, " == 0")
        elseif var == VAR_ONE
            @assert !used
            println(io, " == 1")
        elseif var == VAR_ZERO_OR_ONE
            @assert used
            println(io, " == 0 or 1")
        elseif var == VAR_UNKNOWN
            println(io, ifelse(used, " >= 0", " >= 0 (FREE)"))
        end
    end

    for (i, (var, used)) in enumerate(zip(system.qs, used_q))
        print(io, lpad('q' * string(i), lhs_maxlen + 6))
        if var == VAR_ZERO
            @assert !used
            println(io, " == 0")
        elseif var == VAR_ONE
            @assert !used
            println(io, " == 1")
        elseif var == VAR_ZERO_OR_ONE
            @assert used
            println(io, " == 0 or 1")
        elseif var == VAR_UNKNOWN
            println(io, ifelse(used, " >= 0", " >= 0 (FREE)"))
        end
    end

end


################################################################################


export initial_polynomial, initial_system


function initial_polynomial(m::T, n::T, k::T) where {T<:Integer}
    ZERO = zero(T)
    ONE = one(T)
    TERM_ONE = Term{T}(ZERO, ZERO)
    @assert (ZERO < m < n) && (ZERO < k < m + n)
    a = min(k, m) - ifelse(k > n, k - n, ZERO)
    b = a + ONE
    terms = Vector{Term{T}}(undef, b)
    @inbounds begin
        if k < m
            @simd ivdep for j = ONE:k-ONE
                terms[j] = Term{T}(j, k - j)
            end
            terms[a] = Term{T}(k, ZERO)
            terms[b] = Term{T}(ZERO, k)
        elseif k == m
            @simd ivdep for j = ONE:m-ONE
                terms[j] = Term{T}(j, m - j)
            end
            terms[a] = Term{T}(ZERO, m)
            terms[b] = TERM_ONE
        elseif k < n
            @simd ivdep for j = ONE:m-ONE
                terms[j] = Term{T}(j, k - j)
            end
            terms[a] = Term{T}(ZERO, k - m)
            terms[b] = Term{T}(ZERO, k)
        elseif k == n
            @simd ivdep for j = ONE:m-ONE
                terms[j] = Term{T}(j, n - j)
            end
            terms[a] = Term{T}(ZERO, n - m)
            terms[b] = TERM_ONE
        else
            offset = k - n
            @simd ivdep for j = offset+ONE:m-ONE
                terms[j-offset] = Term{T}(j, k - j)
            end
            terms[a] = Term{T}(k - n, ZERO)
            terms[b] = Term{T}(ZERO, k - m)
        end
    end
    return Polynomial{T}(terms)
end


function initial_system(m::T, n::T) where {T<:Integer}
    ZERO = zero(T)
    ONE = one(T)
    @assert ZERO < m < n
    rhs = [RHS_ZERO_OR_ONE for _ = ONE:m+n-ONE]
    @inbounds rhs[m] = RHS_ONE
    @inbounds rhs[n] = RHS_ONE
    return System{T}(
        [VAR_UNKNOWN for _ = ONE:m-ONE],
        [VAR_UNKNOWN for _ = ONE:n-ONE],
        [initial_polynomial(m, n, k) for k = ONE:m+n-ONE],
        rhs,
    )
end


################################################################################


end # module ZeroOnePolynomials
