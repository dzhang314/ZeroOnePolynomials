module IntervalConstraintPropagation

########################################################## PACKED DATA STRUCTURE


export PackedSystem


const Term = Tuple{Int,Int}
const Equation = Vector{Term}
const System = Vector{Equation}


struct PackedSystem
    offsets::Vector{Int}
    terms::Vector{Term}
    pair_equation::Vector{Int}
    occurrences::Vector{Int}
end


function PackedSystem(system::System)
    # Assign contiguous indices to p and q variables.
    num_terms = 0
    p_map = Dict{Int,Int}(0 => 0)
    q_map = Dict{Int,Int}(0 => 0)
    for equation in system
        num_terms += length(equation)
        for (p, q) in equation
            @assert !(signbit(p) | signbit(q) | (iszero(p) & iszero(q)))
            if !iszero(p)
                get!(p_map, p, length(p_map))
            end
            if !iszero(q)
                get!(q_map, q, length(q_map))
            end
        end
    end
    np = length(p_map) - 1
    nq = length(q_map) - 1
    # We say that p_i and q_j are paired if p_i + q_j occurs in some equation.
    pair_matrix = zeros(Int, np, nq)
    for (e, equation) in enumerate(system)
        for (p, q0) in equation
            if iszero(q0)
                for (p0, q) in equation
                    if iszero(p0)
                        pair_matrix[p_map[p], q_map[q]] = e
                    end
                end
            end
        end
    end
    # Pack equations into flat arrays with unified variable indices.
    offsets = Int[0]
    sizehint!(offsets, length(system) + 1)
    terms = Term[]
    sizehint!(terms, num_terms)
    pair_equation = Int[]
    sizehint!(pair_equation, num_terms)
    occurrences = zeros(Int, np + nq)
    for equation in system
        for (p, q) in equation
            i = p_map[p]
            j = q_map[q]
            if !iszero(i) & !iszero(j)
                e = pair_matrix[i, j]
                j += np
                occurrences[i] += 1
                occurrences[j] += 1
                push!(terms, (i, j))
                push!(pair_equation, e)
            elseif !iszero(i)
                occurrences[i] += 1
                push!(terms, (i, 0))
                push!(pair_equation, 0)
            else
                j += np
                occurrences[j] += 1
                push!(terms, (j, 0))
                push!(pair_equation, 0)
            end
        end
        push!(offsets, length(terms))
    end
    return PackedSystem(offsets, terms, pair_equation, occurrences)
end


############################################################ DIRECTED ARITHMETIC


export add_lo, add_hi, sub_lo, sub_hi, mul_lo, mul_hi, div_lo, div_hi


@inline function _two_sum(x::T, y::T) where {T}
    s = x + y
    x_prime = s - y
    y_prime = s - x_prime
    x_err = x - x_prime
    y_err = y - y_prime
    e = x_err + y_err
    return (s, e)
end


@inline function _two_diff(x::T, y::T) where {T}
    d = x - y
    x_prime = d + y
    y_prime = x_prime - d
    x_err = x - x_prime
    y_err = y - y_prime
    e = x_err - y_err
    return (d, e)
end


@inline function _two_prod(x::T, y::T) where {T}
    p = x * y
    e = fma(x, y, -p)
    return (p, e)
end


@inline function _prev(x::T) where {T}
    S = Base.inttype(T)
    u = reinterpret(S, x)
    v = ifelse(iszero(u), typemin(S) + one(S), u - flipsign(one(S), u))
    return ifelse(x > typemin(T), reinterpret(T, v), x)
end

@inline function _next(x::T) where {T}
    S = Base.inttype(T)
    u = reinterpret(S, x)
    v = ifelse(u == typemin(S), one(S), u + flipsign(one(S), u))
    return ifelse(x < typemax(T), reinterpret(T, v), x)
end


@inline function add_lo(x::T, y::T) where {T}
    s, e = _two_sum(x, y)
    return ifelse(e >= zero(T), s, _prev(s))
end

@inline function add_hi(x::T, y::T) where {T}
    s, e = _two_sum(x, y)
    return ifelse(e <= zero(T), s, _next(s))
end


@inline function sub_lo(x::T, y::T) where {T}
    d, e = _two_diff(x, y)
    return ifelse(e >= zero(T), d, _prev(d))
end

@inline function sub_hi(x::T, y::T) where {T}
    d, e = _two_diff(x, y)
    return ifelse(e <= zero(T), d, _next(d))
end


@inline _underflow_threshold(::Type{T}) where {T} =
    ldexp(one(T), exponent(floatmin(T)) + precision(T))


@inline function mul_lo(x::T, y::T) where {T}
    p, e = _two_prod(x, y)
    correct_sign = iszero(x) | iszero(y) |
        (abs(p) >= _underflow_threshold(T)) | !iszero(e)
    return ifelse(correct_sign & (e >= zero(T)), p, _prev(p))
end

@inline function mul_hi(x::T, y::T) where {T}
    p, e = _two_prod(x, y)
    correct_sign = iszero(x) | iszero(y) |
        (abs(p) >= _underflow_threshold(T)) | !iszero(e)
    return ifelse(correct_sign & (e <= zero(T)), p, _next(p))
end


@inline function div_lo(x::T, y::T) where {T}
    q = x / y
    e = fma(-q, abs(y), flipsign(x, y))
    correct_sign = iszero(x) | (abs(x) >= _underflow_threshold(T)) | !iszero(e)
    return ifelse(correct_sign & (e >= zero(T)), q, _prev(q))
end

@inline function div_hi(x::T, y::T) where {T}
    q = x / y
    e = fma(-q, abs(y), flipsign(x, y))
    correct_sign = iszero(x) | (abs(x) >= _underflow_threshold(T)) | !iszero(e)
    return ifelse(correct_sign & (e <= zero(T)), q, _next(q))
end


####################################################### WORKSPACE DATA STRUCTURE


export BoxWorkspace


struct BoxWorkspace{T}
    box_lo::Vector{T}
    box_hi::Vector{T}
    term_lo::Vector{T}
    term_hi::Vector{T}
end


function BoxWorkspace{T}(packed::PackedSystem) where {T}
    num_variables = length(packed.occurrences)
    max_terms = maximum(diff(packed.offsets); init=0)
    return BoxWorkspace(
        Vector{T}(undef, num_variables),
        Vector{T}(undef, num_variables),
        Vector{T}(undef, max_terms),
        Vector{T}(undef, max_terms))
end


######################################################### CONSTRAINT PROPAGATION


@inline function pair_product_hi(
    packed::PackedSystem,
    e::Int,
    p::Int,
    q::Int,
    box_lo::Vector{T},
) where {T}
    _zero = zero(T)
    _one = one(T)
    _two_lo = add_lo(_one, _one)
    _four_lo = add_lo(_two_lo, _two_lo)
    _quarter_hi = div_hi(_one, _four_lo)
    # Precondition: equation e has the form t1 + ... + tk + p + q = 1.
    # Compute a lower bound r_lo on the remaining terms t1 + ... + tk.
    r_lo = _zero
    for k = (packed.offsets[e]+1):packed.offsets[e+1]
        i, j = packed.terms[k]
        if iszero(j)
            if (i != p) & (i != q)
                r_lo = add_lo(r_lo, box_lo[i])
            end
        else
            r_lo = add_lo(r_lo, mul_lo(box_lo[i], box_lo[j]))
        end
    end
    # If p >= 0, q >= 0, and p + q <= c, then p*q <= c^2/4.
    # This is a simple consequence of the AM-GM inequality.
    c_hi = sub_hi(_one, r_lo)
    return mul_hi(_quarter_hi, mul_hi(c_hi, c_hi))
end


@inline function term_bounds(
    packed::PackedSystem,
    k::Int,
    box_lo::Vector{T},
    box_hi::Vector{T},
) where {T}
    p, q = packed.terms[k]
    if iszero(q)
        return (box_lo[p], box_hi[p])
    end
    lo = mul_lo(box_lo[p], box_lo[q])
    hi = mul_hi(box_hi[p], box_hi[q])
    e = packed.pair_equation[k]
    if !iszero(e)
        hi = min(hi, pair_product_hi(packed, e, p, q, box_lo))
    end
    return (lo, hi)
end


@inline function factor_bounds(
    product_lo::T,
    product_hi::T,
    factor_lo::T,
    factor_hi::T,
) where {T}
    _zero = zero(T)
    _one = one(T)
    # Derive bounds on x from bounds on x*y and bounds on y,
    # assuming 0 <= x <= 1 and 0 <= y <= 1.
    lo = _zero
    hi = _one
    if product_lo > _zero
        if product_lo > factor_hi
            return (_one, _zero)
        end
        lo = div_lo(product_lo, factor_hi)
    end
    if (product_hi < factor_lo) & !iszero(factor_lo)
        hi = div_hi(product_hi, factor_lo)
    end
    return (lo, hi)
end


@inline function tighten!(
    box_lo::Vector{T},
    box_hi::Vector{T},
    i::Int,
    lo::T,
    hi::T,
) where {T}
    _one = one(T)
    _two = _one + _one
    _four = _two + _two
    _eight = _four + _four
    _sixteen = _eight + _eight
    _thirty_two = _sixteen + _sixteen
    _thirty_second = inv(_thirty_two)
    threshold = _thirty_second * (box_hi[i] - box_lo[i])
    result = false
    if lo > box_lo[i]
        result |= (lo - box_lo[i]) > threshold
        box_lo[i] = lo
    end
    if hi < box_hi[i]
        result |= (box_hi[i] - hi) > threshold
        box_hi[i] = hi
    end
    return result
end


@inline function contract_term!(
    box_lo::Vector{T},
    box_hi::Vector{T},
    p::Int,
    q::Int,
    lo::T,
    hi::T,
) where {T}
    if iszero(q)
        changed = tighten!(box_lo, box_hi, p, lo, hi)
        return (changed, box_lo[p] > box_hi[p])
    end
    p_lo, p_hi = factor_bounds(lo, hi, box_lo[q], box_hi[q])
    changed = tighten!(box_lo, box_hi, p, p_lo, p_hi)
    if box_lo[p] > box_hi[p]
        return (changed, true)
    end
    q_lo, q_hi = factor_bounds(lo, hi, box_lo[p], box_hi[p])
    changed |= tighten!(box_lo, box_hi, q, q_lo, q_hi)
    return (changed, box_lo[q] > box_hi[q])
end


################################################################################

end # module IntervalConstraintPropagation
