using TinyDronesSim

@kwdef mutable struct Thruster <: SimObject
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

TinyDronesSim.has_pos(::Thruster) = true
TinyDronesSim.has_mass(::Thruster) = true
TinyDronesSim.has_velocity(::Thruster) = true

function TinyDronesSim.get_mechanical_reaction_local_frame_component(thruster::Thruster)::Resultant3D
    force = (thruster.up * thruster.omega^2) * thruster.lin_coeff_thrust + thruster.lin_coeff_drag .* thruster.u
    torque = thruster.up * (thruster.omega^2 * thruster.lin_coeff_propeller_rotational_drag + thruster.motor_inertia * thruster.omega_dot)
    return Resultant3D(force, torque, Vec3f(0, 0, 0))
end

function TinyDronesSim.get_mechanical_reaction_parent_frame_component(thruster::Thruster)::Resultant3D
    return get_gravity_reaction_parent_frame_component(thruster)
end

function TinyDronesSim.get_inertia_matrix(thruster::Thruster, center_of_mass::Vec3f)::Mat33f
    return get_inertia_matrix_point_mass(thruster, center_of_mass)
end

@kwdef mutable struct Drone <: SimObject
    thruster1::Thruster = Thruster(m=0.1, x=Vec3f(-0.05, +0.05, 0), omega=300) # about 1000 rpm
    thruster2::Thruster = Thruster(m=0.1, x=Vec3f(-0.05, -0.05, 0), omega=300)
    thruster3::Thruster = Thruster(m=0.1, x=Vec3f(+0.05, -0.05, 0), omega=305)
    thruster4::Thruster = Thruster(m=0.1, x=Vec3f(+0.05, +0.05, 0), omega=304)
    mech_point::MechanicalPoint = MechanicalPoint(m=0.3, x=Vec3f(0, 0, 0))
    
    x::Vec3f = Vec3f(0, 0, 0)              # position
    u::Vec3f = Vec3f(0, 0, 0)              # velocity
    r::Quaternion = identity_quaternion()  # orientation
    angular_u::Vec3f = Vec3f(0, 0, 0)      # angular velocity vector [radians / s]
end

function get_inertia_matrix(obj::ObjT) where {ObjT <: SimObject}
    return Mat33f([0.01 0    0;
                   0    0.01 0;
                   0    0    0.01])
end

TinyDronesSim.has_pos(::Drone) = true
TinyDronesSim.has_velocity(::Drone) = true
TinyDronesSim.has_orientation(::Drone) = true
TinyDronesSim.has_angular_velocity(::Drone) = true

function TinyDronesSim.render(ctxid::RenderContextID, drone::Drone)::Nothing
    render(ctxid, get_relative_pos(drone, drone.thruster1))
    render(ctxid, get_relative_pos(drone, drone.thruster2))
    render(ctxid, get_relative_pos(drone, drone.thruster3))
    render(ctxid, get_relative_pos(drone, drone.thruster4))
    render(ctxid, get_relative_pos(drone, drone.mech_point))
end

include("TinyDronesImpl.jl") # this is stupid but very important, will find an alternative hopefully

# Testing

drone = Drone()

ctxid::RenderContextID = init_renderer(RenderConfig())

while !renderer_should_close(ctxid)
    begin_3d_rendering(ctxid)

    for i in 1:10
        integrate_physics_euler!(drone, 0.000167)
    end
    # println(get_pos(drone))
    
    render(ctxid, drone)
    render_grid_floor(ctxid, Int32(60), Float64(1.0))
    end_3d_rendering(ctxid)
end
close_renderer(ctxid)

# t = Vector{Float64}()
# x = Vector{Float64}()
# y = Vector{Float64}()
# z = Vector{Float64}()

# for i in 1:2000
#     integrate_physics_euler!(drone, 0.001)
#     drone_pos = get_pos(drone)
#     push!(t, i)
#     push!(x, drone_pos.x)
#     push!(y, drone_pos.y)
#     push!(z, drone_pos.z)
# end

# using Plots

# display(plot(t, [x, y, z]))
