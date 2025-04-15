using TinyDronesSim
using StaticStrings
const Env = TinyDronesSim.Environments

env = Env.create_environment()

camera = Env.create_camera(env)
camera_motion_state = Env.Camera_Motion_State()

window = Env.create_window(camera, cstatic"drone simulation example", target_fps = Int32(120))

include("simple_drone_declaration.jl")
drone = Drone()
Env.add_renderables(env, drone)

try
    Env.add_ibl_skybox(env, cstatic"/home/yuzeni/projekte/TinyDronesSim/EnvironmentBackend/assets/rogland_sunset_2k.hdr")

    Env.add_unlit_material(env, cstatic"white")
    Env.add_lit_material(env, cstatic"basic_lit")

    # ground platform
    Env.add_plane(env, Float64_3(0, 0, 0), 10.0, 10.0, cstatic"basic_lit")
    Env.add_plane(env, Float64_3(0, 0, 0), 10.0, 10.0, cstatic"basic_lit", quaternion_ccw_x(Float64(pi)))

    run_simulation::Bool = false

    while !Env.window_should_close(window)

        if (Env.is_key_pressed(window, Env.SDL_SCANCODE_SPACE))
            run_simulation = !run_simulation
        end

        if (run_simulation)
            for i in 1:10
                integrate_physics_euler!(drone, (Env.get_last_frame_time_ms(window) / 1000.0) / 10.0, 1e-8)
            end
        end
        
        Env.update_renderables(drone)

        camera_motion_state.orbit_center = get_pos(drone)
        Env.update_camera_by_window_input!(camera, window, camera_motion_state)
        
        Env.update_window(window)
    end

finally
    Env.destroy_everything()
end
