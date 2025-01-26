
module TinyDronesSim

using StaticArrays

export Generic
abstract type Generic end

include("BasicTypes.jl")
include("QuaternionRotation.jl")
include("Metahelpers.")
include("Visualization.jl")
include("Mechanics.jl")

end # module TinyDronesSim
