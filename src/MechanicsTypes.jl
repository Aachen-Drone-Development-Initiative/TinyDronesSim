
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

set_pos!(resultant::Resultant3D, pos::Vec3f) = resultant.pos = pos

function combine_resultants(pos::Vec3f, resultants::Resultant3D...)::Resultant3D
    resultant = Resultant3D(pos=pos)
    for i in 1:length(resultants)
        distance = euclid_norm(resultants[i].pos)
        resultant.force += resultants[i].force
        resultant.torque += resultants[i].torque + cross_product(resultants[i].pos - pos, resultants[i].force)
    end
    return resultant
end

function rotate_resultant(resultant::Resultant3D, rotation::Quaternion)::Resultant3D
    return Resultant3D(rotate(resultant.force, rotation),
                       rotate(resultant.torque, rotation),
                       resultant.pos)
end

### Mechanical Point ###

@kwdef mutable struct MechanicalPoint <: SimObject
    m::Float64 = 0                         # mass
    x::Vec3f = Vec3f(0, 0, 0)              # position
    u::Vec3f = Vec3f(0, 0, 0)              # velocity
    lin_coeff_drag::Vec3f = Vec3f(0, 0, 0)
end

is_renderable(::MechanicalPoint) = true

function TinyDronesSim.render(ctxid::RenderContextID, obj::MechanicalPoint)::Nothing
    render(ctxid, Sphere(obj.x, BLACK, 0.01))
end
