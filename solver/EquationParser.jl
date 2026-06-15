module EquationParser

export print_system, parse_system


const Term = Tuple{Int,Int}
const Equation = Vector{Term}
const System = Vector{Equation}


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


const _ZERO = UInt8('0')

@inline function parse_int(bytes::AbstractVector{UInt8}, i::Int)
    n = lastindex(bytes)
    result = 0
    @inbounds while i <= n
        digit = bytes[i] - _ZERO
        if digit >= 10
            break
        end
        result = muladd(10, result, Int(digit))
        i += 1
    end
    return (result, i)
end


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


end # module EquationParser
