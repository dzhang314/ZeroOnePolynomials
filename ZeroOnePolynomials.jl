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


export Term


struct Term{T<:Integer}
    p_index::T
    q_index::T
end


@inline function Base.isone(term::Term{T}) where {T<:Integer}
    return iszero(term.p_index) && iszero(term.q_index)
end


@inline function is_p(term::Term{T}) where {T<:Integer}
    return (!iszero(term.p_index)) && iszero(term.q_index)
end


@inline function is_q(term::Term{T}) where {T<:Integer}
    return iszero(term.p_index) && (!iszero(term.q_index))
end


@inline function is_quadratic(term::Term{T}) where {T<:Integer}
    return (!iszero(term.p_index)) && (!iszero(term.q_index))
end


function Base.show(io::IO, term::Term{T}) where {T<:Integer}
    p_index = term.p_index
    q_index = term.q_index
    if iszero(p_index)
        if iszero(q_index)
            print(io, '1')
        else
            print(io, 'q')
            print(io, q_index)
        end
    else
        print(io, 'p')
        print(io, p_index)
        if !iszero(q_index)
            print(io, '*')
            print(io, 'q')
            print(io, q_index)
        end
    end
    return nothing
end


################################################################################


export Polynomial


struct Polynomial{T<:Integer}
    terms::Vector{Term{T}}
end


function Base.show(io::IO, polynomial::Polynomial{T}) where {T<:Integer}
    if isempty(polynomial.terms)
        print(io, '0')
    else
        first = true
        for term in polynomial.terms
            if first
                first = false
            else
                print(io, " + ")
            end
            print(io, term)
        end
    end
    return nothing
end


################################################################################


export System


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

    lhs_strs = string.(system.lhs)
    lhs_maxlen = maximum(length(s) for s in lhs_strs)

    for (i, (lhs_str, rhs)) in enumerate(zip(lhs_strs, system.rhs))
        if (lhs_str == "0") && (rhs == RHS_ZERO)
            continue
        end
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
        if (var == VAR_ZERO) || (var == VAR_ONE)
            @assert !used
        elseif var == VAR_ZERO_OR_ONE
            @assert used
            print(io, lpad('p' * string(i), lhs_maxlen + 6))
            println(io, " == 0 or 1")
        elseif var == VAR_UNKNOWN
            print(io, lpad('p' * string(i), lhs_maxlen + 6))
            println(io, ifelse(used, " >= 0", " >= 0 (FREE)"))
        end
    end

    for (i, (var, used)) in enumerate(zip(system.qs, used_q))
        if (var == VAR_ZERO) || (var == VAR_ONE)
            @assert !used
        elseif var == VAR_ZERO_OR_ONE
            @assert used
            print(io, lpad('q' * string(i), lhs_maxlen + 6))
            println(io, " == 0 or 1")
        elseif var == VAR_UNKNOWN
            print(io, lpad('q' * string(i), lhs_maxlen + 6))
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
    ONE = one(T)
    @assert zero(T) < m < n
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


export set_p_zero!, set_q_zero!, set_p_one!, set_q_one!


function set_p_zero!(system::System{T}, p_index::T) where {T<:Integer}
    @assert system.ps[p_index] != VAR_ONE
    system.ps[p_index] = VAR_ZERO
    for polynomial in system.lhs
        filter!(term -> term.p_index != p_index, polynomial.terms)
    end
    return system
end


function set_q_zero!(system::System{T}, q_index::T) where {T<:Integer}
    @assert system.qs[q_index] != VAR_ONE
    system.qs[q_index] = VAR_ZERO
    for polynomial in system.lhs
        filter!(term -> term.q_index != q_index, polynomial.terms)
    end
    return system
end


function set_p_one!(system::System{T}, p_index::T) where {T<:Integer}
    @assert system.ps[p_index] != VAR_ZERO
    system.ps[p_index] = VAR_ONE
    @inbounds for polynomial in system.lhs
        terms = polynomial.terms
        @simd ivdep for i in eachindex(terms)
            term = terms[i]
            terms[i] = Term{T}(
                ifelse(term.p_index == p_index, zero(T), term.p_index),
                term.q_index
            )
        end
    end
    return system
end


function set_q_one!(system::System{T}, q_index::T) where {T<:Integer}
    @assert system.qs[q_index] != VAR_ZERO
    system.qs[q_index] = VAR_ONE
    @inbounds for polynomial in system.lhs
        terms = polynomial.terms
        @simd ivdep for i in eachindex(terms)
            term = terms[i]
            terms[i] = Term{T}(
                term.p_index,
                ifelse(term.q_index == q_index, zero(T), term.q_index)
            )
        end
    end
    return system
end


################################################################################


function initial_system(m::T, n::T, case_index::UInt64) where {T<:Integer}
    ONE = one(T)
    @assert zero(T) < m < n
    system = initial_system(m, n)
    set_q_zero!(system, m)
    set_q_zero!(system, n - m)
    for j = ONE:m-ONE
        if iszero(case_index & one(UInt64))
            set_p_zero!(system, j)
        else
            set_q_zero!(system, m - j)
            set_q_zero!(system, n - j)
        end
        case_index >>= one(UInt64)
    end
    return system
end


################################################################################


export simplify!


function is_unknown(system::System{T}, term::Term{T}) where {T<:Integer}
    p_index = term.p_index
    if !iszero(p_index)
        if system.ps[p_index] == VAR_UNKNOWN
            return true
        end
    end
    q_index = term.q_index
    if !iszero(q_index)
        if system.qs[q_index] == VAR_UNKNOWN
            return true
        end
    end
    return false
end


function simplify!(system::System{T}) where {T<:Integer}

    @assert length(system.lhs) == length(system.rhs)
    lone_quadratic_terms = Term{T}[]
    for (i, (polynomial, rhs)) in enumerate(zip(system.lhs, system.rhs))

        # If an equation has no terms on its left-hand side,
        # then set its right-hand side to zero.
        if isempty(polynomial.terms)
            if rhs == RHS_ONE
                return false
            elseif rhs != RHS_ZERO
                system.rhs[i] = RHS_ZERO
                return simplify!(system)
            end
        end

        # If an equation has only one term on its left-hand side,
        # and that term is not quadratic, then simplification may be possible.
        if isone(length(polynomial.terms))
            term = only(polynomial.terms)

            # If the term on the left-hand side is one,
            # then set the right-hand side to one.
            if isone(term)
                if rhs == RHS_ZERO
                    return false
                elseif rhs != RHS_ONE
                    system.rhs[i] = RHS_ONE
                    return simplify!(system)
                end
            end

            # If the term on the left-hand side is a variable p_i,
            # then set p_i equal to the right-hand side.
            if is_p(term)
                p_index = term.p_index
                p_value = system.ps[p_index]
                @assert p_value != VAR_ZERO
                @assert p_value != VAR_ONE
                if rhs == RHS_ZERO
                    set_p_zero!(system, p_index)
                    return simplify!(system)
                elseif rhs == RHS_ONE
                    set_p_one!(system, p_index)
                    return simplify!(system)
                elseif rhs == RHS_ZERO_OR_ONE
                    if p_value != VAR_ZERO_OR_ONE
                        system.ps[p_index] = VAR_ZERO_OR_ONE
                        return simplify!(system)
                    end
                end
            end

            # Do the same for variables of the form q_i.
            if is_q(term)
                q_index = term.q_index
                q_value = system.qs[q_index]
                @assert q_value != VAR_ZERO
                @assert q_value != VAR_ONE
                if rhs == RHS_ZERO
                    set_q_zero!(system, q_index)
                    return simplify!(system)
                elseif rhs == RHS_ONE
                    set_q_one!(system, q_index)
                    return simplify!(system)
                elseif rhs == RHS_ZERO_OR_ONE
                    if q_value != VAR_ZERO_OR_ONE
                        system.qs[q_index] = VAR_ZERO_OR_ONE
                        return simplify!(system)
                    end
                end
            end

            # Keep track of lone quadratic terms for later use.
            if is_quadratic(term)
                push!(lone_quadratic_terms, term)
            end
        end

        # If the term 1 occurs on the left-hand side of an equation,
        # then its right-hand side must also be one.
        # We delete the term and set the right-hand side to zero.
        # If there are multiple such terms, then the system is unsatisfiable.
        one_index = 0
        for (j, term) in enumerate(polynomial.terms)
            if isone(term)
                if !iszero(one_index)
                    return false
                end
                one_index = j
            end
        end
        if !iszero(one_index)
            if rhs == RHS_ZERO
                return false
            end
            system.rhs[i] = RHS_ZERO
            deleteat!(polynomial.terms, one_index)
            return simplify!(system)
        end

        # If all terms but one on the left-hand side of an equation have been
        # constrained to zero or one, then the same holds for the final term.
        unknown_index = 0
        for (j, term) in enumerate(polynomial.terms)
            if is_unknown(system, term)
                if !iszero(unknown_index)
                    unknown_index = 0
                    break
                end
                unknown_index = j
            end
        end
        if !iszero(unknown_index)
            unknown_term = polynomial.terms[unknown_index]
            if is_p(unknown_term)
                p_index = unknown_term.p_index
                @assert system.ps[p_index] == VAR_UNKNOWN
                system.ps[p_index] = VAR_ZERO_OR_ONE
                return simplify!(system)
            end
            if is_q(unknown_term)
                q_index = unknown_term.q_index
                @assert system.qs[q_index] == VAR_UNKNOWN
                system.qs[q_index] = VAR_ZERO_OR_ONE
                return simplify!(system)
            end
            if is_quadratic(unknown_term)
                push!(lone_quadratic_terms, unknown_term)
            end
        end
    end

    made_change = false
    for polynomial in system.lhs
        if length(polynomial.terms) == 2
            x = polynomial.terms[1]
            y = polynomial.terms[2]
            if is_p(x) && is_q(y)
                p_index = x.p_index
                q_index = y.q_index
                term = Term{T}(p_index, q_index)
                if term in lone_quadratic_terms
                    p_value = system.ps[p_index]
                    @assert p_value != VAR_ZERO
                    @assert p_value != VAR_ONE
                    q_value = system.qs[q_index]
                    @assert q_value != VAR_ZERO
                    @assert q_value != VAR_ONE
                    if p_value != VAR_ZERO_OR_ONE
                        system.ps[p_index] = VAR_ZERO_OR_ONE
                        made_change = true
                    end
                    if q_value != VAR_ZERO_OR_ONE
                        system.qs[q_index] = VAR_ZERO_OR_ONE
                        made_change = true
                    end
                end
            end
            if is_p(y) && is_q(x)
                p_index = y.p_index
                q_index = x.q_index
                term = Term{T}(p_index, q_index)
                if term in lone_quadratic_terms
                    p_value = system.ps[p_index]
                    @assert p_value != VAR_ZERO
                    @assert p_value != VAR_ONE
                    q_value = system.qs[q_index]
                    @assert q_value != VAR_ZERO
                    @assert q_value != VAR_ONE
                    if p_value != VAR_ZERO_OR_ONE
                        system.ps[p_index] = VAR_ZERO_OR_ONE
                        made_change = true
                    end
                    if q_value != VAR_ZERO_OR_ONE
                        system.qs[q_index] = VAR_ZERO_OR_ONE
                        made_change = true
                    end
                end
            end
        end
    end
    if made_change
        return simplify!(system)
    end

    return true
end


################################################################################


export initial_systems


function initial_systems(m::T, n::T) where {T<:Integer}
    result = [
        initial_system(m, n, case_index)
        for case_index = UInt64(0):((UInt64(1)<<(m-one(T)))-one(UInt64))
    ]
    for system in result
        @assert simplify!(system)
    end
    return result
end


################################################################################


end # module ZeroOnePolynomials
