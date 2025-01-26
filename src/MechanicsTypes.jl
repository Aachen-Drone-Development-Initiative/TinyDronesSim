
export gravitational_acceleration,
    Resultant3D,
    combine_resultants,
    rotate_resultant,
    
    MechanicalPoint

### Constants ###

const gravitational_acceleration = 9.81 # [m/s^2]

### Resultant in 3D ###

@kwdef mutable struct Resultant3D
    force::Vec3f = Vec3f(0, 0, 0)
    torque::Vec3f = Vec3f(0, 0, 0)
    pos::Vec3f = Vec3f(0, 0, 0)
end

### Mechanical Point ###

@kwdef mutable struct MechanicalPoint
    m::Float64 = 0                         # mass
    x::Vec3f = Vec3f(0, 0, 0)              # position
    u::Vec3f = Vec3f(0, 0, 0)              # velocity
    lin_coeff_drag::Vec3f = Vec3f(0, 0, 0)
end
