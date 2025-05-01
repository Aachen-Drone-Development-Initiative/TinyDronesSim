using TinyDronesSim
using StaticStrings
using Printf
const Env = TinyDronesSim.Environments

include("simple_drone_declaration.jl")

# Adding all the object that we require for the simulation and visualization

env = Env.create_environment()
Env.add_ibl_skybox(cstatic"./EnvironmentBackend/assets/rogland_sunset_2k.hdr")

camera = Env.create_camera(env, far_plane = 300.0)
camera_motion_state = Env.Camera_Motion_State()
window = Env.create_window(camera, cstatic"drone simulation example", target_fps = 60)

drone_camera = Env.create_camera(env)
window_drone_pov = Env.create_window(drone_camera, cstatic"drone POV")

drone = Drone()
add_renderables(drone)

cool_gltf_asset = Env.add_gltf_asset_and_create_instance(cstatic"./EnvironmentBackend/assets/castle.glb");

# joystick setup

Env.window_activate(window) # The joystick input should be captured by the main window

if Env.connect_to_joystick()
    println("INFO: Connected to joystick.")
else
    println("ERROR: Failed to connect to joystick, please plug in your joystick and call 'Env.connect_to_joystick()'.")
end

println("------
After your joystick is connected, it needs to be calibrated!
First run 'joy_calib_code = Env.generate_joystick_calibration_code()', which will
return Julia code that you then need to execute like this 'eval(calibration_code)'
------")

function sim_loop()

    Env.window_show(window)
    Env.window_show(window_drone_pov)

    run_simulation = false

    # When the window is closed or minimized, it's considered to be invisible
    while Env.window_visible(window) || Env.window_visible(window_drone_pov)

        if Env.window_visible(window)
            Env.window_activate(window)

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

            throttle = (Env.get_joystick_axis_mapped_value(Env.JOYSTICK_THROTTLE) + 1.0) * 30.0
            yaw = Env.get_joystick_axis_mapped_value(Env.JOYSTICK_YAW) * 30.0
            pitch = Env.get_joystick_axis_mapped_value(Env.JOYSTICK_PITCH) * 30.0
            roll = Env.get_joystick_axis_mapped_value(Env.JOYSTICK_ROLL) * 30.0

            # Very rudimentary mapping between drone and joystick
            drone.thrusters[1].angular_velo = -abs(throttle + pitch - roll - yaw)
            drone.thrusters[2].angular_velo = abs(throttle - pitch - roll + yaw)
            drone.thrusters[3].angular_velo = abs(throttle + pitch + roll + yaw)
            drone.thrusters[4].angular_velo = -abs(throttle - pitch + roll - yaw)

            @printf("angular velocity of the thrusters: t1 ω: %g t2 ω: %g t3 ω: %g t4 ω: %g            \r",
                    drone.thrusters[1].angular_velo, drone.thrusters[2].angular_velo, drone.thrusters[3].angular_velo, drone.thrusters[4].angular_velo)

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

        if Env.window_visible(window_drone_pov)
            Env.window_activate(window_drone_pov)

            # "attach" the drone_camera to the drone
            Env.set_position(Env.get_filament_entity(drone_camera), get_pos(drone))
            # FIXME: The orientation is for some reason not correct, using the 'conjugate' "fixes" things, but this is obviously not ideal
            Env.set_orientation(Env.get_filament_entity(drone_camera), conjugate(get_orientation(drone)))
            
            Env.window_update()
        end
    end

end
    
