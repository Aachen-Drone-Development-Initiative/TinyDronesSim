using TinyDronesSim
const tds = TinyDronesSim

include("simple_drone_declaration.jl")

# Adding all the object that we require for the simulation and visualization

# joystick setup
joystick = tds.joystick_create_and_connect("/dev/input/js1")
if invalid_id(joystick) exit(1) end

println("#########
After your joystick is connected, it needs to be calibrated!
Simply do:
tds.joystick_calibrate(joystick)

Though you should rather use the function to generate the calibration code and copy it here.
This is done like this (in the repl):
tds.joystick_calibrate(my_joystick, joystick_symbol = :my_joystick)
########")

env = tds.create_environment()
tds.env_add_ibl_skybox("./TinyDroneSimBackend/assets/rogland_sunset_2k.hdr")

camera = tds.camera_create(env, far_plane = 300.0)
camera_motion_state = tds.Camera_Motion_State()

drone_camera = tds.camera_create(env)

drone = Drone()
add_renderables(env, drone)

gltf_asset = tds.env_add_gltf_asset_and_create_instance("./TinyDroneSimBackend/assets/castle.glb");

viewport = tds.viewport_create()

tds.viewport_add_controll_panel(viewport)
tds.viewport_add_camera(viewport, camera, camera_update, tds.VIEWPORT_UPPER)
tds.viewport_add_camera(viewport, drone_camera, drone_camera_update, [tds.VIEWPORT_UPPER, tds.VIEWPORT_LEFT])

function camera_update(update::tds.Viewport_Camera_Callback_Update)::UInt32
    
end

function drone_camera_update(update::tds.Viewport_Camera_Callback_Update)::UInt32
    
end

function sim_loop()

    tds.window_show(window)
    tds.window_show(window_drone_pov)

    run_simulation = false

    # When the window is closed or minimized, it's considered to be invisible
    while tds.window_visible(window) || tds.window_visible(window_drone_pov)

        if tds.window_visible(window)
            tds.window_activate(window)

            if (tds.is_key_pressed(tds.SDL_SCANCODE_SPACE))
                run_simulation = !run_simulation
            end

            # reset the drone
            if (tds.is_key_pressed(tds.SDL_SCANCODE_R))
                set_pos!(drone, Float64_3(0,0,0))
                set_velocity!(drone, Float64_3(0,0,0))
                set_orientation!(drone, identity_quaternion())
                set_angular_velocity!(drone, Float64_3(0,0,0))
            end

            throttle = (tds.get_joystick_axis_mapped_value(tds.JOYSTICK_THROTTLE) + 1.0) * 30.0
            yaw = tds.get_joystick_axis_mapped_value(tds.JOYSTICK_YAW) * 30.0
            pitch = tds.get_joystick_axis_mapped_value(tds.JOYSTICK_PITCH) * 30.0
            roll = tds.get_joystick_axis_mapped_value(tds.JOYSTICK_ROLL) * 30.0

            # Very rudimentary mapping between drone and joystick
            drone.thrusters[1].angular_velo = -abs(throttle + pitch - roll - yaw)
            drone.thrusters[2].angular_velo = abs(throttle - pitch - roll + yaw)
            drone.thrusters[3].angular_velo = abs(throttle + pitch + roll + yaw)
            drone.thrusters[4].angular_velo = -abs(throttle - pitch + roll - yaw)

            @printf("angular velocity of the thrusters: t1 ω: %g t2 ω: %g t3 ω: %g t4 ω: %g            \r",
                    drone.thrusters[1].angular_velo, drone.thrusters[2].angular_velo, drone.thrusters[3].angular_velo, drone.thrusters[4].angular_velo)

            if (run_simulation)
                for i in 1:100
                    integrate_physics_euler!(drone, (tds.window_get_last_frame_time_ms() / 1000.0) / 100.0, 1e-8)
                end
            end
            
            update_renderables(drone)
            
            camera_motion_state.orbit_center = get_pos(drone) # center the camera around the drone
            tds.update_camera_by_window_input!(camera, camera_motion_state, mode=tds.THIRD_PERSON) # update the camera by mouse and keyboard input
            
            tds.window_update()
        end

        if tds.window_visible(window_drone_pov)
            tds.window_activate(window_drone_pov)

            # "attach" the drone_camera to the drone
            tds.set_position(tds.get_filament_entity(drone_camera), get_pos(drone))
            # FIXME: The orientation is for some reason not correct, using the 'conjugate' "fixes" things, but this is obviously not ideal
            tds.set_orientation(tds.get_filament_entity(drone_camera), conjugate(get_orientation(drone)))
            
            tds.window_update()
        end
    end

end
    
