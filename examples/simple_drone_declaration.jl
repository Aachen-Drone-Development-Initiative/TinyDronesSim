using TinyDronesSim
using StaticArrays
const TDS = TinyDronesSim
const Env = TinyDronesSim.Environments

# The Thruster struct contains all the variables defining a thruster
@kwdef mutable struct Thruster
    pwm::Int64 = 0
    angular_velo::Float64 = 0.0 # [radians per second];  positive -> counter-clock-wise, negative -> clock-wise
    angular_accel::Float64 = 0.0
    pos::Float64_3 = Float64_3(0.0, 0.0, 0.0) # relative position to the drone
end

# most constants here are just guesses (does it look okay)
@kwdef mutable struct Drone
    mass::Float64 = 0.8
    lin_coeff_thrust::Float64 = 0.0001 # proportional to (rotational speed)^2
    lin_coeff_propeller_rotational_drag::Float64 = 0.00001 # proportional to (rotational speed)^2
    lin_coeff_drag::Float64_3 = Float64_3(1.0, 1.0, 1.0)
    up::Float64_3 = Float64_3(0.0, 1.0, 0.0) # a vector defining the 'up' direction of the drone
    motor_inertia::Float64 = 0.0
    
    thrusters::SVector{4, Thruster} = (Thruster(angular_velo=-300, pos=Float64_3(+0.05, 0, +0.05)),
                                       Thruster(angular_velo=300, pos=Float64_3(+0.05, 0, -0.05)),
                                       Thruster(angular_velo=155, pos=Float64_3(-0.05, 0, +0.05)),
                                       Thruster(angular_velo=-155, pos=Float64_3(-0.05, 0, -0.05)))
    pos::Float64_3 = Float64_3(0.0, 0.0, 0.0)             # position
    velo::Float64_3 = Float64_3(0.0, 0.0, 0.0)            # velocity
    orientation::Quaternion = identity_quaternion()
    angular_velo::Float64_3 = Float64_3(0.0, 0.0, 0.0)    # angular velocity vector [radians / s]

    # stuff for visualization
    associated_environment::Env.UUID = 0
    gltf_model_instance::Env.UUID = 0
    trail::Vector{Float32_3} = Vector{Float32_3}(undef, 400) # For rendering (and especially with GPUs) 32-bit floats are the standard
end

# Based on: https://andrew.gibiansky.com/downloads/pdf/Quadcopter%20Dynamics,%20Simulation,%20and%20Control.pdf
function TDS.get_mechanical_reaction(drone::Drone)::Resultant3D
    force = -drone.velo .* drone.lin_coeff_drag # simple air resistance
    torque = Float64_3(0, 0, 0)
    for i in 1:4
        # thrust from the propellers
        motor_force = (drone.up * drone.thrusters[i].angular_velo^2) * drone.lin_coeff_thrust
        force += motor_force
        # roll and pitch torque
        torque += cross_product(motor_force, drone.thrusters[i].pos)
        # yaw torque: rotational drag of propellers and acceleration of motors
        torque += drone.up * (sign(drone.thrusters[i].angular_velo) * drone.thrusters[i].angular_velo^2 * drone.lin_coeff_propeller_rotational_drag
                              + drone.motor_inertia * drone.thrusters[i].angular_accel)
    end
    
    # rotate the force and torque from the local to the global frame of reference
    force = rotate_vector(force, drone.orientation)
    torque = rotate_vector(torque, drone.orientation)

    # add gravity force
    force += Float64_3(0, 0, -drone.mass * gravitational_acceleration)
    
    return Resultant3D(force, torque, drone.pos)
end

function TDS.get_inertia_matrix(drone::Drone)::Float64_3x3
    return Float64_3x3([0.05 0    0
                        0    0.05 0
                        0    0    0.05])
end

TDS.get_pos(drone::Drone)::Float64_3              = drone.pos
TDS.get_mass(drone::Drone)::Float64               = drone.mass
TDS.get_velocity(drone::Drone)::Float64_3         = drone.velo
TDS.get_orientation(drone::Drone)::Quaternion     = drone.orientation
TDS.get_angular_velocity(drone::Drone)::Float64_3 = drone.angular_velo
TDS.set_pos!(drone::Drone, pos::Float64_3)                       = drone.pos = pos
TDS.set_velocity!(drone::Drone, velo::Float64_3)                 = drone.velo = velo
TDS.set_orientation!(drone::Drone, orientation::Quaternion)      = drone.orientation = orientation
TDS.set_angular_velocity!(drone::Drone, angular_velo::Float64_3) = drone.angular_velo = angular_velo

function Env.add_renderables(env_id::Env.UUID, drone::Drone)
    drone.associated_environment = env_id
    drone.gltf_model_instance = Env.add_gltf_asset_and_create_instance(env_id, cstatic"/home/yuzeni/projekte/TinyDronesSim/EnvironmentBackend/assets/TinyDroneEspS3.glb");
end

function Env.update_renderables(drone::Drone)
    prev_drone_pos = Env.get_position(drone.gltf_model_instance)
    Env.set_position_and_orientation(drone.gltf_model_instance, get_pos(drone), get_orientation(drone))

    # this is a retarded method for rendering the trail
    Env.add_line(drone.associated_environment, prev_drone_pos, get_pos(drone), cstatic"white")
end
