
module TinyDronesSim

using StaticArrays

export SimObject
abstract type SimObject end

include("Utils.jl")
include("Visualization.jl")
include("Mechanics.jl")

end # module TinyDronesSim b
