using TinyDronesSim
using StaticArrays
const tds = TinyDronesSim

# @kwdef required for parameter initialization inside the struct declaration
@kwdef mutable struct Thruster
    pwm::Int64 = 0
    angular_u::Float64 = 0    # rotational speed of propeller [radians per second];  positive -> counter-clock-wise, negative -> clock-wise
    angular_a::Float64 = 0    # rotational acceleration
    x::Vec3f = Vec3f(0, 0, 0) # relative position to the drone
end

@kwdef mutable struct Drone
    # constant parameters
    mass::Float64 = 0.8
    lin_coeff_thrust::Float64 = 0.0001                # proportional to (rotational speed)^2
    lin_coeff_propeller_rotational_drag::Float64 = 0  # proportional to (rotational speed)^2
    lin_coeff_drag::Vec3f = Vec3f(0, 0, 0)
    up::Vec3f = Vec3f(0, 0, 1)                        # a vector defining the 'up' direction of the drone
    motor_inertia::Float64 = 0

    # variable parameters
    thrusters::SVector{4, Thruster} = (Thruster(angular_u=300, x=Vec3f(0.05, 0.05, 0)),
                                       Thruster(angular_u=250, x=Vec3f(0.05, -0.05, 0)),
                                       Thruster(angular_u=145, x=Vec3f(-0.05, 0.05, 0)),
                                       Thruster(angular_u=145, x=Vec3f(-0.05, -0.05, 0)))
    x::Vec3f = Vec3f(0, 0, 0)              # position
    u::Vec3f = Vec3f(0, 0, 0)              # velocity
    r::Quaternion = identity_quaternion()  # orientation
    angular_u::Vec3f = Vec3f(0, 0, 0)      # angular velocity vector [radians / s]

    # stuff for visualization
    trail::Vector{Vec3f_32} = Vector{Vec3f_32}(undef, 400) # For rendering (and especially with GPUs) 32-bit floats are the standard
end

function tds.get_mechanical_reaction(drone::Drone)::Resultant3D
    force = -drone.u .* drone.lin_coeff_drag # simple air resistance
    torque = Vec3f(0, 0, 0)
    for i in 1:4
        # thrust from the propellers
        motor_force = (drone.up * drone.thrusters[i].angular_u^2) * drone.lin_coeff_thrust
        force += motor_force
        # roll and pitch torque
        torque += cross_product(motor_force, drone.thrusters[i].x)
        # yaw torque: rotational drag of propellers and acceleration of motors
        torque += drone.up * (drone.thrusters[i].angular_u^2 * drone.lin_coeff_propeller_rotational_drag + drone.motor_inertia * drone.thrusters[i].angular_a)
    end
    
    # rotate the force and torque from the local to the global frame of reference
    force = rotate(force, drone.r)
    torque = rotate(torque, drone.r)

    # add gravity force
    force += Vec3f(0, 0, -drone.mass * gravitational_acceleration)
    
    return Resultant3D(force, torque, drone.x)
end

function tds.get_inertia_matrix(drone::Drone)::Mat33f
    return Mat33f([0.1 0   0
                   0   0.1 0
                   0   0   0.1])
end

tds.get_pos(drone::Drone)::Vec3f              = drone.x
tds.get_mass(drone::Drone)::Float64           = drone.mass
tds.get_velocity(drone::Drone)::Vec3f         = drone.u
tds.get_orientation(drone::Drone)::Quaternion = drone.r
tds.get_angular_velocity(drone::Drone)::Vec3f = drone.angular_u
tds.set_pos!(drone::Drone, x::Vec3f)                      = drone.x = x
tds.set_velocity!(drone::Drone, u::Vec3f)                 = drone.u = u
tds.set_orientation!(drone::Drone, r::Quaternion)         = drone.r = r
tds.set_angular_velocity!(drone::Drone, angular_u::Vec3f) = drone.angular_u = angular_u

function tds.render(ctxid::RenderContextID, drone::Drone)::Nothing
    # render the center of gravity and the thruster positions with the default render function for Vec3f
    render(ctxid, drone.x)
    for i in 1:4
        render(ctxid, relative_to_absolute_pos(drone, drone.thrusters[i].x))
    end
    # render the trail of the drone
    for i in 1:(length(drone.trail) - 1)
        drone.trail[i] = drone.trail[i + 1]
    end
    drone.trail[end] = drone.x
    render_path(ctxid, drone.trail, length(drone.trail), MAGENTA)
end

# Testing

drone = Drone() # Create a 'Drone' object

ctxid::RenderContextID = init_renderer(RenderConfig()) # Initialize the renderer with the default configuration

# The rendering application loop:
# repeat until the window should be closed (ESC or X in the right corner)
while !renderer_should_close(ctxid)
    
    # Do 10 timesteps per frame, at 60 FPS (the dafault config)
    # with a timestep of 1 / (60 * 100) = 0.0001667, the sim runs at 1/10 of realtime
    for i in 1:10
        integrate_physics_euler!(drone, 0.0001667, 1e-8)
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
