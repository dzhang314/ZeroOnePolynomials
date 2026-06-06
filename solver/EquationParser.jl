module EquationParser

export print_canonical_system, load_pq_systems, load_canonical_systems


const Term = Tuple{Int,Int}
const Equation = Vector{Term}
const System = Vector{Equation}


function print_canonical_term(io::IO, i::Int, j::Int)
    print(io, 'x')
    print(io, i)
    if !iszero(j)
        print(io, "*x")
        print(io, j)
    end
    return nothing
end


function print_canonical_equation(io::IO, equation::Equation)
    if isempty(equation)
        print(io, '0')
    else
        first_term = true
        for (i, j) in equation
            if first_term
                first_term = false
            else
                print(io, " + ")
            end
            print_canonical_term(io, i, j)
        end
    end
    return nothing
end


function print_canonical_system(io::IO, system::System)
    for equation in system
        print_canonical_equation(io, equation)
        print(io, '\n')
    end
    return nothing
end


const _ZERO = UInt8('0')
const _NINE = UInt8('9')

@inline function parse_int(bytes::Vector{UInt8}, i::Int)
    n = length(bytes)
    result = 0
    @inbounds while i <= n
        b = bytes[i]
        if !((b >= _ZERO) & (b <= _NINE))
            break
        end
        result = muladd(10, result, Int(b - _ZERO))
        i += 1
    end
    return (result, i)
end


const _P = UInt8('p')
const _Q = UInt8('q')
const _STAR = UInt8('*')

@inline function parse_pq_term(bytes::Vector{UInt8}, i::Int)
    n = length(bytes)
    @inbounds begin
        head = bytes[i]
        if head == _P
            p, i = parse_int(bytes, i + 1)
            if (i < n) && (bytes[i] == _STAR)
                @assert bytes[i+1] == _Q
                q, i = parse_int(bytes, i + 2)
                return ((p, q), i)
            else
                return ((p, 0), i)
            end
        else
            @assert head == _Q
            q, i = parse_int(bytes, i + 1)
            return ((0, q), i)
        end
    end
end


const _X = UInt8('x')

@inline function parse_canonical_term(bytes::Vector{UInt8}, i::Int)
    n = length(bytes)
    @inbounds begin
        @assert bytes[i] == _X
        x, i = parse_int(bytes, i + 1)
        if (i < n) && (bytes[i] == _STAR)
            @assert bytes[i+1] == _X
            y, i = parse_int(bytes, i + 2)
            return ((x, y), i)
        else
            return ((x, 0), i)
        end
    end
end


const _PLUS = UInt8('+')
const _SPACE = UInt8(' ')
const _NEWLINE = UInt8('\n')

function load_systems(parse_term::F, path::AbstractString) where {F}
    bytes = read(path)
    n = length(bytes)
    equation = Term[]
    system = Equation[]
    result = System[]
    i = 1
    @inbounds while i <= n
        b = bytes[i]
        if (b == _PLUS) | (b == _SPACE)
            i += 1
        elseif b == _NEWLINE
            if isempty(equation)
                push!(result, system)
                system = Equation[]
                i += 1
            else
                push!(system, equation)
                equation = Term[]
                i += 1
            end
        else
            term, i = parse_term(bytes, i)
            push!(equation, term)
        end
    end
    @assert isempty(equation)
    @assert isempty(system)
    return result
end

load_pq_systems(path::AbstractString) = load_systems(parse_pq_term, path)

load_canonical_systems(path::AbstractString) =
    load_systems(parse_canonical_term, path)


end # module EquationParser
