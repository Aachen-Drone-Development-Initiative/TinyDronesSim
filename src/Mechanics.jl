
#####################
# Mechanics Interface
#####################

# Define these functions for your types to make them compatible with the functionality provided in Mechanics.jl.
# Not all functions need to be defined, errors will tell you if something is missing.

function position(obj::Nothing)::Float64_3 end
function mass(obj::Nothing)::Float64 end
function velocity(obj::Nothing)::Float64_3 end
function orientation(obj::Nothing)::Quaternion end
function angular_velocity(obj::Nothing)::Float64_3 end

function set_position!(obj::Nothing, x::Float64_3) end
function set_mass!(obj::Nothing, m::Float64) end
function set_velocity!(obj::Nothing, u::Float64_3) end
function set_orientation!(obj::Nothing, r::Quaternion) end
function set_angular_velocity!(obj::Nothing, angular_u::Float64_3) end

function inertia_matrix(obj::Nothing) end
function force_and_torque(obj::Nothing) end

const c_gravitational_acceleration = 9.81 # [m/s^2]

#####################
# Numerical integration
#####################

"""
Update the position and orientation of an object, based on its 'mechanical reaction'
using the simple euler integration method (https://en.wikipedia.org/wiki/Euler_method)
"""
function mech_integrate_force_and_torque_euler!(obj, dt)
    force, torque = force_and_torque(obj)

    # update velocity, position
    m = mass(obj)
    u_dot = force / m

    set_velocity!(obj, velocity(obj) + u_dot * dt)
    set_position!(obj, position(obj) + velocity(obj) * dt)
    
    # update angular_velocity, orientation
    I = inertia_matrix(obj)
    angular_u = angular_velocity(obj)
    angular_u_dot = inv(I) * (torque - vec_cross(angular_u, I * angular_u))
    set_angular_velocity!(obj, angular_u + angular_u_dot * dt)
    
    orientation_delta = angular_velocity(obj) * dt
    set_orientation!(obj, vec_normalize(quat_combine(orientation(obj), quat_from_axis_angle(orientation_delta))))
end
