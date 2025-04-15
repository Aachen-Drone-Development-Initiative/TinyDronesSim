using TinyDronesSim
const TDS = TinyDronesSim

include("simple_drone_declaration.jl")

# Requires Makie, any Makie-Backend should work, this example uses the OpenGL-Backend, add with 'Pkg.add("GLMakie")'
# For example, if you are on the web use WGLMakie, the WebGL-Backend

using GLMakie

function get_renderable(drone::Drone)
    thrusters_and_center = Vector{TDS.Vec3f}()
    push!(thrusters_and_center, drone.x)
    for i in 1:4
        push!(thrusters_and_center, relative_to_absolute_pos(drone, drone.thrusters[i].x))
    end
    return thrusters_and_center
end

function next_sim_step(drone::Drone)
    for i in 1:10
        integrate_physics_euler!(drone, 0.0001667, 1e-8)
    end
end

drone = Drone()

# Makie uses 'Observable' for interactive plots: https://docs.makie.org/stable/explanations/observables
drone_renderable = Observable(get_renderable(drone))

figure = Figure()
display(figure)
axis = Axis3(figure[1,1])

plot = meshscatter!(axis, drone_renderable, markersize = 0.01, framerate = 60)

while isopen(figure.scene)
    next_sim_step(drone)
    drone_renderable[] = get_renderable(drone) # update the observable
    sleep(0.1)
end
