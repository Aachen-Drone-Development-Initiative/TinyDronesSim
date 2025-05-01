const TDS = TinyDronesSim

export get_pos,
    get_mass,
    get_orientation,
    get_velocity,
    get_angular_velocity,
    get_inertia_matrix,
    get_force_and_torque

export set_pos!,
    set_mass!,
    set_orientation!,
    set_velocity!,
    set_angular_velocity!

export gravitational_acceleration,
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
function get_force_and_torque(obj::Nothing) end

const gravitational_acceleration = 9.81 # [m/s^2]

### Mechanics Functionality ###

"""
Update the position and orientation of an object, based on its 'mechanical reaction'
using the simple euler integration method (https://en.wikipedia.org/wiki/Euler_method)
"""
function integrate_physics_euler!(obj, dt::Float64, epsilon::Float64)
    force, torque = get_force_and_torque(obj)

    # update velocity, position
    mass = get_mass(obj)
    u_dot = force / mass

    set_velocity!(obj, get_velocity(obj) + u_dot * dt)
    set_pos!(obj, get_pos(obj) + get_velocity(obj) * dt)
    
    # update angular_velocity, orientation
    I = get_inertia_matrix(obj)
    angular_u = get_angular_velocity(obj)
    angular_u_dot = inv(I) * (torque - cross_product(angular_u, I * angular_u))
    set_angular_velocity!(obj, angular_u + angular_u_dot * dt)
    
    orientation_delta = get_angular_velocity(obj) * dt
    set_orientation!(obj, normalize(combine(get_orientation(obj), axis_angle_to_quaternion(orientation_delta, epsilon))))
end
