
module TinyDronesSim

using StaticArrays
using StaticStrings

include("./BasicTypes.jl")
include("./QuaternionRotation.jl")
include("./Mechanics.jl")

module Environments
include("./Environment.jl")
end # module Environments

end # module TinyDronesSim
