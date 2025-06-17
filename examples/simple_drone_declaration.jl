using TinyDronesSim
using StaticArrays
const tds = TinyDronesSim

@kwdef mutable struct Thruster
    pwm::Int64 = 0
    angular_velo::Float64 = 0.0 # [radians / s]; positive -> counter-clock-wise rotation, negative -> clock-wise rotation
    angular_accel::Float64 = 0.0
    pos::tds.Float64_3 = tds.Float64_3(0.0, 0.0, 0.0) # relative position to the drone
end

@kwdef mutable struct Drone
    mass::Float64 = 0.5
    lin_coeff_thrust::Float64 = 0.0001 # proportional to (rotational speed)^2
    lin_coeff_propeller_rotational_drag::Float64 = 0.00001 # proportional to (rotational speed)^2
    lin_coeff_drag::tds.Float64_3 = tds.Float64_3(0.001, 0.001, 0.001)
    up::tds.Float64_3 = tds.Float64_3(0.0, 1.0, 0.0) # a vector defining the 'up' direction of the drone
    motor_inertia::Float64 = 0.0
    
    thrusters::Vector{Thruster} = [Thruster(angular_velo=-300, pos=tds.Float64_3(+0.05, 0, +0.05)),
                                   Thruster(angular_velo=300, pos=tds.Float64_3(+0.05, 0, -0.05)),
                                   Thruster(angular_velo=155, pos=tds.Float64_3(-0.05, 0, +0.05)),
                                   Thruster(angular_velo=-155, pos=tds.Float64_3(-0.05, 0, -0.05))]
    
    pos::tds.Float64_3 = tds.Float64_3(0.0, 0.0, 0.0)
    velo::tds.Float64_3 = tds.Float64_3(0.0, 0.0, 0.0)
    orientation::tds.Quaternion = tds.quat_identity()
    angular_velo::tds.Float64_3 = tds.Float64_3(0.0, 0.0, 0.0) # [radians / s]

    gltf_model_instance::tds.glTF_Instance_ID = tds.glTF_Instance_ID() # 3d model of the drone
end

# Based on: https://andrew.gibiansky.com/downloads/pdf/Quadcopter%20Dynamics,%20Simulation,%20and%20Control.pdf
function tds.force_and_torque(drone::Drone)
    force = tds.Float64_3(0, 0, 0)
    torque = tds.Float64_3(0, 0, 0)
    
    for i in 1:4
        motor_force = (drone.up * drone.thrusters[i].angular_velo^2) * drone.lin_coeff_thrust # thrust from the propellers
        force += motor_force
        torque += tds.vec_cross(motor_force, drone.thrusters[i].pos) # roll and pitch torque
        
        # yaw torque: rotational drag of propellers and acceleration of motors
        torque += drone.up * (sign(drone.thrusters[i].angular_velo) * drone.thrusters[i].angular_velo^2 * drone.lin_coeff_propeller_rotational_drag
                              + drone.motor_inertia * drone.thrusters[i].angular_accel)
    end
    
    # rotate the force and torque from the local to the global frame of reference
    force = tds.quat_rotate_vector(force, drone.orientation)
    torque = tds.quat_rotate_vector(torque, drone.orientation)

    # add gravity force
    force += tds.Float64_3(0, -drone.mass * tds.c_gravitational_acceleration, 0)
    
    return force, torque
end

function tds.inertia_matrix(drone::Drone)::tds.Float64_3x3
    return tds.Float64_3x3([0.01 0    0
                            0    0.01 0
                            0    0    0.01])
end

tds.position(drone::Drone)::tds.Float64_3         = drone.pos
tds.mass(drone::Drone)::Float64                   = drone.mass
tds.velocity(drone::Drone)::tds.Float64_3         = drone.velo
tds.orientation(drone::Drone)::tds.Quaternion         = drone.orientation
tds.angular_velocity(drone::Drone)::tds.Float64_3 = drone.angular_velo
tds.set_position!(drone::Drone, pos::tds.Float64_3)                  = drone.pos = pos
tds.set_velocity!(drone::Drone, velo::tds.Float64_3)                 = drone.velo = velo
tds.set_orientation!(drone::Drone, orientation::tds.Quaternion)      = drone.orientation = orientation
tds.set_angular_velocity!(drone::Drone, angular_velo::tds.Float64_3) = drone.angular_velo = angular_velo

function drone_add_renderables(env::tds.Environment_ID, drone::Drone)
    tds.env_activate(env)
    tds.env_add_unlit_material("unlit_white") # The default parameters generate a plain white material
    drone.gltf_model_instance = tds.env_add_gltf_asset_and_create_instance("./TinyDronesSimBackend/assets/TinyDroneEspS3.glb");
end

function drone_update_renderables(env::tds.Environment_ID, drone::Drone)
    tds.env_activate(env)
    prev_drone_pos = tds.gltf_inst_position(drone.gltf_model_instance)
    
    # FIXME: The orientation is for some reason not correct, using the 'conjugate' "fixes" things, but this is obviously not ideal
    tds.gltf_inst_set_position_and_orientation(drone.gltf_model_instance, tds.position(drone), tds.quat_conjugate(tds.orientation(drone)))

    # this is a retarded method for rendering the trail
    tds.env_add_line(prev_drone_pos, tds.position(drone), "unlit_white")
end
