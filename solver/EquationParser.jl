module EquationParser

export print_system, load_systems


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

@inline function parse_term(bytes::Vector{UInt8}, i::Int)
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


const _PLUS = UInt8('+')
const _SPACE = UInt8(' ')
const _NEWLINE = UInt8('\n')

const _BLOCK_SIZE = 1 << 20

function parse_systems!(io::IO, channel::Channel{System})
    block = Vector{UInt8}(undef, _BLOCK_SIZE)
    bytes = UInt8[]
    equation = Term[]
    system = Equation[]
    while true
        num_bytes = readbytes!(io, block, _BLOCK_SIZE)
        append!(bytes, view(block, 1:num_bytes))
        done = eof(io)
        n = done ? length(bytes) : something(findlast(==(_NEWLINE), bytes), 0)
        i = 1
        @inbounds while i <= n
            b = bytes[i]
            if (b == _PLUS) | (b == _SPACE)
                i += 1
            elseif b == _NEWLINE
                if isempty(equation)
                    put!(channel, system)
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
        deleteat!(bytes, 1:n)
        if done
            break
        end
    end
    @assert isempty(bytes)
    @assert isempty(equation)
    @assert isempty(system)
    return nothing
end


const _CHANNEL_SIZE = 1 << 10

load_systems(path::AbstractString) = Channel{System}(channel ->
        open(io -> parse_systems!(io, channel), path), _CHANNEL_SIZE)


end # module EquationParser
