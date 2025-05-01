using TinyDronesSim
using StaticStrings
using Printf
const Env = TinyDronesSim.Environments

include("simple_drone_declaration.jl")

# Adding all the object which we require for the simulation and visualization
env = Env.create_environment()
Env.add_ibl_skybox(cstatic"./EnvironmentBackend/assets/rogland_sunset_2k.hdr")

camera = Env.create_camera(env, far_plane = 300.0)
camera_motion_state = Env.Camera_Motion_State()
window = Env.create_window(camera, cstatic"drone simulation example", target_fps = 60)

drone = Drone()
add_renderables(drone)

cool_gltf_asset = Env.add_gltf_asset_and_create_instance(cstatic"./EnvironmentBackend/assets/castle.glb");

function sim_loop()
    Env.window_show(window)
    run_simulation = false

    # When the window is closed or minimized, it's considered to be invisible
    while Env.window_visible(window)

        if (Env.is_key_pressed(Env.SDL_SCANCODE_SPACE))
            run_simulation = !run_simulation
        end

        # reset the drone
        if (Env.is_key_pressed(Env.SDL_SCANCODE_R))
            set_pos!(drone, Float64_3(0,0,0))
            set_velocity!(drone, Float64_3(0,0,0))
            set_orientation!(drone, identity_quaternion())
            set_angular_velocity!(drone, Float64_3(0,0,0))
        end

        if (run_simulation)
            for i in 1:100
                integrate_physics_euler!(drone, (Env.window_get_last_frame_time_ms() / 1000.0) / 100.0, 1e-8)
            end
        end
        
        update_renderables(drone)
        
        camera_motion_state.orbit_center = get_pos(drone) # center the camera around the drone
        Env.update_camera_by_window_input!(camera, camera_motion_state, mode=Env.THIRD_PERSON) # update the camera by mouse and keyboard input
        
        Env.window_update()
    end
end
    
