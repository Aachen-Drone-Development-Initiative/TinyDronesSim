using TinyDronesSim
const tds = TinyDronesSim

@kwdef mutable struct Thruster <: Generic
    omega::Float64 = 0     # rotational speed of propeller [radians per second]
                           # negative -> clock-wise, positive -> counter-clock-wise
    omega_dot::Float64 = 0 # rotational acceleration of propeller
    up::Vec3f = (0, 0, 1)
    lin_coeff_thrust::Float64 = 0.0001 # proportional to (rotational speed)^2
    lin_coeff_propeller_rotational_drag::Float64 = 0 # proportional to (rotational speed)^2
    motor_inertia::Float64 = 0

    m::Float64 = 0                         # mass
    x::Vec3f = Vec3f(0, 0, 0)              # position
    u::Vec3f = Vec3f(0, 0, 0)              # velocity
    lin_coeff_drag::Vec3f = Vec3f(0, 0, 0)
end

tds.get_pos(thruster::Thruster)::Vec3f      = thruster.x
tds.get_mass(thruster::Thruster)::Float64   = thruster.m
tds.get_velocity(thruster::Thruster)::Vec3f = thruster.u
tds.set_pos!(thruster::Thruster, x::Vec3f)      = thruster.x = x
tds.set_mass!(thruster::Thruster, m::Float64)   = thruster.m = m
tds.set_velocity!(thruster::Thruster, u::Vec3f) = thruster.u = u

function tds.get_mechanical_reaction_local_frame_component(thruster::Thruster)::Resultant3D
    force = (thruster.up * thruster.omega^2) * thruster.lin_coeff_thrust + thruster.lin_coeff_drag .* thruster.u
    torque = thruster.up * (thruster.omega^2 * thruster.lin_coeff_propeller_rotational_drag + thruster.motor_inertia * thruster.omega_dot)
    return Resultant3D(force, torque, Vec3f(0, 0, 0))
end

function tds.get_mechanical_reaction_parent_frame_component(thruster::Thruster)::Resultant3D
    return get_gravity_reaction_parent_frame_component(thruster)
end

function tds.get_inertia_matrix(thruster::Thruster, center_of_mass::Vec3f)::Mat33f
    return get_inertia_matrix_point_mass(thruster, center_of_mass)
end

@kwdef mutable struct Drone <: Generic
    thruster1::Thruster = Thruster(m=0.1, x=Vec3f(-0.05, +0.05, 0), omega=105) # about 1000 rpm
    thruster2::Thruster = Thruster(m=0.1, x=Vec3f(-0.05, -0.05, 0), omega=105)
    thruster3::Thruster = Thruster(m=0.1, x=Vec3f(+0.05, -0.05, 0), omega=107)
    thruster4::Thruster = Thruster(m=0.1, x=Vec3f(+0.05, +0.05, 0), omega=109)
    mech_point::MechanicalPoint = MechanicalPoint(m=0.3, x=Vec3f(0, 0, 0))
    
    x::Vec3f = Vec3f(0, 0, 0)              # position
    u::Vec3f = Vec3f(0, 0, 0)              # velocity
    r::Quaternion = identity_quaternion()  # orientation
    angular_u::Vec3f = Vec3f(0, 0, 0)      # angular velocity vector [radians / s]

    # stuff for visualization
    trail::Vector{Vec3f_32} = Vector{Vec3f_32}(undef, 400) # For rendering (and especially with GPUs) 32-bit floats are the standard
end

tds.get_pos(drone::Drone)::Vec3f              = drone.x
tds.get_velocity(drone::Drone)::Vec3f         = drone.u
tds.get_orientation(drone::Drone)::Quaternion = drone.r
tds.get_angular_velocity(drone::Drone)::Vec3f = drone.angular_u
tds.set_pos!(drone::Drone, x::Vec3f)                      = drone.x = x
tds.set_velocity!(drone::Drone, u::Vec3f)                 = drone.u = u
tds.set_orientation!(drone::Drone, r::Quaternion)         = drone.r = r
tds.set_angular_velocity!(drone::Drone, angular_u::Vec3f) = drone.angular_u = angular_u

function tds.render(ctxid::RenderContextID, drone::Drone)::Nothing
    render(ctxid, get_relative_pos(drone, drone.thruster1))
    render(ctxid, get_relative_pos(drone, drone.thruster2))
    render(ctxid, get_relative_pos(drone, drone.thruster3))
    render(ctxid, get_relative_pos(drone, drone.thruster4))
    render(ctxid, get_relative_pos(drone, drone.mech_point))
    
    for i in 1:(length(drone.trail) - 1)
        drone.trail[i] = drone.trail[i + 1]
    end
    drone.trail[end] = get_relative_pos(drone, drone.mech_point)
    render_path(ctxid, drone.trail, length(drone.trail), MAGENTA)
end

include("../src/TinyDronesImpl.jl") # this is stupid but very important, will find an alternative hopefully

# Testing

drone = Drone()

# Initialize the renderer with the default configuration
ctxid::RenderContextID = init_renderer(RenderConfig())

# The rendering application loop
while !renderer_should_close(ctxid)
    
    # Do 100 timesteps per frame, at 60 FPS (the dafault config) and a dt 0.000167,
    # the sim runs at realtime
    for i in 1:100
        integrate_physics_euler!(drone, 0.000167)
    end

    # render all the stuff
    begin_3d_rendering(ctxid)
    begin
        render(ctxid, drone)
        render_grid_floor(ctxid, 60, 1.0)
    end
    end_3d_rendering(ctxid)
end
close_renderer(ctxid)
