const tds = TinyDronesSim

export get_pos,
    get_mass,
    get_orientation,
    get_velocity,
    get_angular_velocity,
    set_pos!,
    set_mass!,
    set_orientation!,
    set_velocity!,
    set_angular_velocity!,

    get_inertia_matrix,
    get_mechanical_reaction,

    gravitational_acceleration,

    Resultant3D,
    combine_resultants,
    rotate_resultant,

    relative_to_absolute_pos,
    integrate_physics_euler!

### Mechanics Interface ###

# Define these functions for your custom types to make them compatible with the functionality provided here.
# Not all functions need to be defined, errors will tell you if there is missing something.

function get_pos(obj::Nothing)::Vec3f end
function get_mass(obj::Nothing)::Float64 end
function get_velocity(obj::Nothing)::Vec3f end
function get_orientation(obj::Nothing)::Quaternion end
function get_angular_velocity(obj::Nothing)::Vec3f end

function set_pos!(obj::Nothing, x::Vec3f) end
function set_mass!(obj::Nothing, m::Float64) end
function set_velocity!(obj::Nothing, u::Vec3f) end
function set_orientation!(obj::Nothing, r::Quaternion) end
function set_angular_velocity!(obj::Nothing, angular_u::Vec3f) end

function get_inertia_matrix(obj::Nothing) end
function get_mechanical_reaction(obj::Nothing) end

### Mechanics Constants ###

const gravitational_acceleration = 9.81 # [m/s^2]

### Mechanics Types ###

@kwdef mutable struct Resultant3D
    force::Vec3f = Vec3f(0, 0, 0)
    torque::Vec3f = Vec3f(0, 0, 0)
    pos::Vec3f = Vec3f(0, 0, 0)
end

tds.set_pos!(resultant::Resultant3D, pos::Vec3f) = resultant.pos = pos

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

### Mechanics Functionality ###

function relative_to_absolute_pos(obj, relative_pos::Vec3f)
    return get_pos(obj) + rotate(relative_pos, get_orientation(obj))
end

# Update the position and orientation of an object, based on its 'mechanical reaction'
# using the simple euler integration method (https://en.wikipedia.org/wiki/Euler_method)
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
    set_orientation!(obj, combine(get_orientation(obj), axis_angle_to_quaternion(orientation_delta, epsilon)))
end
