using TinyDronesSim
using StaticStrings
using Printf
const Env = TinyDronesSim.Environments

env = Env.create_environment()

camera = Env.create_camera(env)
camera_motion_state = Env.Camera_Motion_State()

window = Env.create_window(camera, cstatic"drone simulation example", target_fps = 60)

drone_camera = Env.create_camera(env)
window_drone_pov = Env.create_window(drone_camera, cstatic"drone POV")

include("simple_drone_declaration.jl")
drone = Drone()
add_renderables(drone)

try
    Env.add_ibl_skybox(cstatic"./EnvironmentBackend/assets/rogland_sunset_2k.hdr")
    cool_gltf_asset = Env.add_gltf_asset_and_create_instance(cstatic"./EnvironmentBackend/assets/castle.glb");

    # joystick setup

    Env.set_active_window(window)
    
    if Env.connect_to_joystick()
        println("Connected to joystick")
    end

    # Call 'Env.generate_joystick_calibration_code()' to generate the below calibration for your controller/joystick.
    
    # GENERATED Calibration for my rc-radio
    if !(Env.is_connected_to_joystick())
        println("Connect to joystick before executing the calibration routine.")
    else
        Env.assign_joystick_axis_idx_to_axis_type(0x03, Env.JOYSTICK_THROTTLE)
        Env.set_joystick_axis_range(Env.JOYSTICK_THROTTLE, -19325, 2184, -4194)
        Env.assign_joystick_axis_idx_to_axis_type(0x05, Env.JOYSTICK_YAW)
        Env.set_joystick_axis_range(Env.JOYSTICK_YAW, -19325, 2184, -3509)
        Env.assign_joystick_axis_idx_to_axis_type(0x02, Env.JOYSTICK_PITCH)
        Env.set_joystick_axis_range(Env.JOYSTICK_PITCH, -19325, 2184, -3529)
        Env.assign_joystick_axis_idx_to_axis_type(0x01, Env.JOYSTICK_ROLL)
        Env.set_joystick_axis_range(Env.JOYSTICK_ROLL, -19325, 2184, -3529)
    end
    # END GENERATED calibration

    run_simulation = false

    while Env.exists(window) || Env.exists(window_drone_pov)

        if Env.exists(window)
            Env.set_active_window(window)

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

            throttle = (Env.get_joystick_axis_mapped_value(Env.JOYSTICK_THROTTLE) + 1.0) * 100.0
            yaw = Env.get_joystick_axis_mapped_value(Env.JOYSTICK_YAW) * 100.0
            pitch = Env.get_joystick_axis_mapped_value(Env.JOYSTICK_PITCH) * 100.0
            roll = Env.get_joystick_axis_mapped_value(Env.JOYSTICK_ROLL) * 100.0

            # Very rudimentary mapping between drone and joystick
            drone.thrusters[1].angular_velo = -abs(throttle + pitch - roll - yaw)
            drone.thrusters[2].angular_velo = abs(throttle - pitch - roll + yaw)
            drone.thrusters[3].angular_velo = abs(throttle + pitch + roll + yaw)
            drone.thrusters[4].angular_velo = -abs(throttle - pitch + roll - yaw)

            @printf("angular velocity of the thrusters: t1 ω: %g t2 ω: %g t3 ω: %g t4 ω: %g            \r",
                    drone.thrusters[1].angular_velo, drone.thrusters[2].angular_velo, drone.thrusters[3].angular_velo, drone.thrusters[4].angular_velo)

            if (run_simulation)
                for i in 1:100
                    integrate_physics_euler!(drone, (Env.get_last_frame_time_of_window_ms() / 1000.0) / 100.0, 1e-8)
                end
            end
            
            update_renderables(drone)
            
            camera_motion_state.orbit_center = get_pos(drone) # center the camera around the drone
            Env.update_camera_by_window_input!(camera, camera_motion_state, mode=Env.THIRD_PERSON) # update the camera by mouse and keyboard input
            
            Env.update_window()
        end

        if Env.exists(window_drone_pov)
            Env.set_active_window(window_drone_pov)

            # "attach" the drone_camera to the drone
            Env.set_position(drone_camera, get_pos(drone))
            # FIXME: The orientation is for some reason not correct, using the 'conjugate' "fixes" things, but this is obviously not ideal
            Env.set_orientation(drone_camera, conjugate(get_orientation(drone)))
            
            Env.update_window()
        end
    end
    
finally
    Env.destroy_everything()
end
