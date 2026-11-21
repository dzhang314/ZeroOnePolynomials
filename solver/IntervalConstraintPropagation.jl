module IntervalConstraintPropagation

######################################################### PACKED DATA STRUCTURES


export PackedSystem


const Term = Tuple{Int,Int}
const Equation = Vector{Term}
const System = Vector{Equation}


struct PackedSystem
    offsets::Vector{Int}
    terms::Vector{Term}
    paired_factors::Vector{Bool}
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
            @assert !(signbit(p) | signbit(q))
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
    paired = falses(np, nq)
    for equation in system
        for (p, q0) in equation
            if !iszero(p) & iszero(q0)
                for (p0, q) in equation
                    if iszero(p0) & !iszero(q)
                        paired[p_map[p], q_map[q]] = true
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
    paired_factors = Bool[]
    sizehint!(paired_factors, num_terms)
    occurrences = zeros(Int, np + nq)
    for equation in system
        for (p, q) in equation
            i = p_map[p]
            j = q_map[q]
            if !iszero(i) & !iszero(j)
                push!(paired_factors, paired[i, j])
                j += np
                occurrences[i] += 1
                occurrences[j] += 1
                push!(terms, (i, j))
            elseif !iszero(i)
                occurrences[i] += 1
                push!(terms, (i, 0))
                push!(paired_factors, false)
            elseif !iszero(j)
                j += np
                occurrences[j] += 1
                push!(terms, (j, 0))
                push!(paired_factors, false)
            else
                push!(terms, (0, 0))
                push!(paired_factors, false)
            end
        end
        push!(offsets, length(terms))
    end
    return PackedSystem(offsets, terms, paired_factors, occurrences)
end


################################################################################

end # module IntervalConstraintPropagation
