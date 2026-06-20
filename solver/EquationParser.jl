module EquationParser

##################################################################### DATA TYPES


const Term = Tuple{Int,Int}
const Equation = Vector{Term}
const System = Vector{Equation}


####################################################################### PRINTING


export print_system


function print_term(io::IO, p::Int, q::Int)
    if iszero(p)
        if iszero(q)
            print(io, '1')
        else
            print(io, 'q')
            print(io, q)
        end
    else
        print(io, 'p')
        print(io, p)
        if !iszero(q)
            print(io, "*q")
            print(io, q)
        end
    end
    return nothing
end


function print_equation(io::IO, equation::Equation)
    if isempty(equation)
        print(io, '0')
    else
        first_term = true
        for (p, q) in equation
            if first_term
                first_term = false
            else
                print(io, " + ")
            end
            print_term(io, p, q)
        end
    end
    return nothing
end


function print_system(io::IO, system::System)
    for equation in system
        print_equation(io, equation)
        print(io, '\n')
    end
    return nothing
end


################################################################ NUMERIC PARSING


const _ZERO = UInt8('0')

@inline function parse_integer(
    ::Type{T},
    fold_digit::F,
    bytes::AbstractVector{UInt8},
    i::Int,
) where {F,T}
    n = lastindex(bytes)
    result = zero(T)
    @inbounds while i <= n
        digit = bytes[i] - _ZERO
        if digit >= 10
            break
        end
        result = fold_digit(result, digit)
        i += 1
    end
    return (result, i)
end


@inline fold_digit_int(accumulator::Int, digit::UInt8) =
    muladd(10, accumulator, Int(digit))

@inline parse_int(bytes::AbstractVector{UInt8}, i::Int) =
    parse_integer(Int, fold_digit_int, bytes, i)


############################################################### SYMBOLIC PARSING


export parse_system


const _P = UInt8('p')
const _Q = UInt8('q')
const _STAR = UInt8('*')

@inline function parse_term(bytes::AbstractVector{UInt8}, i::Int)
    n = lastindex(bytes)
    @inbounds begin
        head = bytes[i]
        if head == _P
            p, i = parse_int(bytes, i + 1)
            if (i <= n) && (bytes[i] == _STAR)
                @assert (i < n) && (bytes[i+1] == _Q)
                q, i = parse_int(bytes, i + 2) # skip "*q"
                return ((p, q), i)
            else
                return ((p, 0), i)
            end
        else
            @assert head == _Q
            q, i = parse_int(bytes, i + 1) # skip 'q'
            return ((0, q), i)
        end
    end
end


const _PLUS = UInt8('+')
const _SPACE = UInt8(' ')
const _NEWLINE = UInt8('\n')

function parse_equation(bytes::AbstractVector{UInt8}, i::Int)
    n = lastindex(bytes)
    result = Term[]
    @inbounds while i <= n
        b = bytes[i]
        if b == _NEWLINE
            return (result, i + 1)
        elseif (b == _PLUS) | (b == _SPACE)
            i += 1
        else
            term, i = parse_term(bytes, i)
            push!(result, term)
        end
    end
    throw(ArgumentError("equation must be terminated by '\\n'"))
end


function parse_system(bytes::AbstractVector{UInt8}, i::Int)
    n = lastindex(bytes)
    result = Equation[]
    @inbounds while i <= n
        b = bytes[i]
        if b == _NEWLINE
            return (result, i + 1)
        else
            equation, i = parse_equation(bytes, i)
            push!(result, equation)
        end
    end
    throw(ArgumentError("system must be terminated by a blank line"))
end


############################################################# MODULAR ARITHMETIC


export ModM61


const M61 = (one(UInt64) << 61) - one(UInt64)


struct ModM61

    value::UInt64

    @inline function ModM61(x::UInt64)
        r = (x & M61) + (x >> 61)
        return new(ifelse(r >= M61, r - M61, r))
    end

    @inline global unsafe_m61(x::UInt64) = new(x)

end


@inline function Base.:+(x::ModM61, y::ModM61)
    s = x.value + y.value
    return unsafe_m61(ifelse(s >= M61, s - M61, s))
end


@inline function Base.:-(x::ModM61, y::ModM61)
    d = x.value - y.value
    return unsafe_m61(ifelse(x.value < y.value, d + M61, d))
end


@inline function Base.:*(x::ModM61, y::ModM61)
    p = widemul(x.value, y.value)
    lo = (p % UInt64) & M61
    hi = (p >> 61) % UInt64
    return unsafe_m61(lo) + unsafe_m61(hi)
end


@inline pow_pow2(x::T, ::Val{0}) where {T} = x
@inline pow_pow2(x::T, ::Val{N}) where {T,N} = pow_pow2(x * x, Val{N - 1}())


@inline function Base.inv(x::ModM61)
    x2 = x * x
    x3 = x2 * x
    x4 = x2 * x2
    x7 = x4 * x3
    x11 = x7 * x4
    x22 = x11 * x11
    x29 = x22 * x7
    x58 = x29 * x29
    x116 = x58 * x58
    x127 = x116 * x11
    x16383 = pow_pow2(x127, Val{7}()) * x127
    u = pow_pow2(x16383, Val{5}()) # x^(2^19 - 32)
    v = pow_pow2(u, Val{14}()) * u # x^(2^33 - 32)
    w = pow_pow2(v, Val{28}()) * v # x^(2^61 - 32)
    return w * x29                 # x^(2^61 - 3)
end


################################################################################

end # module EquationParser
