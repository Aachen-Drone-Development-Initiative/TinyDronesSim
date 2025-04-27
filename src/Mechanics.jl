const TDS = TinyDronesSim

export get_pos,
    get_mass,
    get_orientation,
    get_velocity,
    get_angular_velocity,
    set_pos!,
    set_mass!,
    set_orientation!,
    set_velocity!,
    set_angular_velocity!

export get_inertia_matrix,
    get_mechanical_reaction

export gravitational_acceleration

export Resultant3D,
    combine_resultants,
    rotate_resultant

export relative_to_absolute_pos,
    integrate_physics_euler!

### Mechanics Interface ###

# Define these functions for your types to make them compatible with the functionality provided in Mechanics.jl.
# Not all functions need to be defined, errors will tell you if something is missing.

function get_pos(obj::Nothing)::Float64_3 end
function get_mass(obj::Nothing)::Float64 end
function get_velocity(obj::Nothing)::Float64_3 end
function get_orientation(obj::Nothing)::Quaternion end
function get_angular_velocity(obj::Nothing)::Float64_3 end

function set_pos!(obj::Nothing, x::Float64_3) end
function set_mass!(obj::Nothing, m::Float64) end
function set_velocity!(obj::Nothing, u::Float64_3) end
function set_orientation!(obj::Nothing, r::Quaternion) end
function set_angular_velocity!(obj::Nothing, angular_u::Float64_3) end

function get_inertia_matrix(obj::Nothing) end
function get_mechanical_reaction(obj::Nothing) end

### Mechanics Constants ###

const gravitational_acceleration = 9.81 # [m/s^2]

### Mechanics Types ###

# A 'resultant' is a 'force' and a 'torque' acting at a 'pos'
@kwdef mutable struct Resultant3D
    force::Float64_3 = Float64_3(0, 0, 0)
    torque::Float64_3 = Float64_3(0, 0, 0)
    pos::Float64_3 = Float64_3(0, 0, 0)
end

TDS.set_pos!(resultant::Resultant3D, pos) = resultant.pos = pos

"Combine resutants and move them to 'pos'"
function combine_resultants(pos, resultants::Resultant3D...)::Resultant3D
    resultant = Resultant3D(pos=pos)
    for i in 1:length(resultants)
        resultant.force += resultants[i].force
        resultant.torque += resultants[i].torque + cross_product(resultants[i].pos - pos, resultants[i].force)
    end
    return resultant
end

function rotate_resultant(resultant::Resultant3D, rotation::Quaternion)::Resultant3D
    return Resultant3D(rotate_vector(resultant.force, rotation),
                       rotate_vector(resultant.torque, rotation),
                       resultant.pos)
end

### Mechanics Functionality ###

"Transform a position vector which is relative to 'obj', to the same coordinate-space 'obj' is relative to."
function relative_to_absolute_pos(obj, relative_pos)
    return get_pos(obj) + rotate_vector(relative_pos, get_orientation(obj))
end

"""
Update the position and orientation of an object, based on its 'mechanical reaction'
using the simple euler integration method (https://en.wikipedia.org/wiki/Euler_method)
"""
function integrate_physics_euler!(obj, dt::Float64, epsilon::Float64)
    resultant = get_mechanical_reaction(obj)

    # update velocity, position
    mass = get_mass(obj)
    u_dot = resultant.force / mass

    set_velocity!(obj, get_velocity(obj) + u_dot * dt)
    set_pos!(obj, get_pos(obj) + get_velocity(obj) * dt)
    
    # update angular_velocity, orientation
    I = get_inertia_matrix(obj)
    angular_u = get_angular_velocity(obj)
    angular_u_dot = inv(I) * (resultant.torque - cross_product(angular_u, I * angular_u))
    set_angular_velocity!(obj, angular_u + angular_u_dot * dt)
    
    orientation_delta = get_angular_velocity(obj) * dt
    set_orientation!(obj, normalize(combine(get_orientation(obj), axis_angle_to_quaternion(orientation_delta, epsilon))))
end
