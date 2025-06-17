using TinyDronesSim
const tds = TinyDronesSim

include("simple_drone_declaration.jl")

env = tds.env_create() # creating an environment, things like lights and meshes live here
tds.env_add_ibl_skybox("./TinyDronesSimBackend/assets/rogland_sunset_2k.hdr") # add skybox to the active environment (which is the last created env)

drone = Drone() # create a drone as specified in "./simple_drone_declaration.jl"
drone_add_renderables(env, drone)

cool_gltf_asset = tds.env_add_gltf_asset_and_create_instance("./TinyDronesSimBackend/assets/castle.glb");

camera = tds.camera_create(env, far_plane = 300.0) # add a camera to the env

@kwdef mutable struct Update_Sim
    run_simulation = false
    space_key_down = false # using this to determine if space key was 'pressed'
    camera_motion_state = tds.Camera_Motion_State() # tracking some addtional camera info to properly update the cameras position and orientation
end    

function update(update::tds.Viewport_Camera_Callback_Update)::UInt32
    tds.viewport_activate(update.viewport)

    update_obj = unsafe_pointer_to_objref(update.user_ptr)::Update_Sim

    # reset the drone while the key 'r' is down
    if (tds.viewport_key_down('r'))
        tds.set_position!(drone, tds.Float64_3(0,0,0))
        tds.set_velocity!(drone, tds.Float64_3(0,0,0))
        tds.set_orientation!(drone, tds.quat_identity())
        tds.set_angular_velocity!(drone, tds.Float64_3(0,0,0))
    end

    # pause/continue the simulation when 'space' is down this call
    # and was not down the previous call (indicating a 'press')
    if (tds.viewport_key_down(' ') && !update_obj.space_key_down)
        update_obj.run_simulation = !update_obj.run_simulation
        update_obj.space_key_down = true
    else
        update_obj.space_key_down = false
    end

    # simulation
    dt_s = update.delta_time_us * 1e-6
    if (update_obj.run_simulation)
        for i in 1:100
            tds.mech_integrate_force_and_torque_euler!(drone, dt_s / 100.0)
        end
    end
    
    drone_update_renderables(env, drone)

    update_obj.camera_motion_state.orbit_center = tds.position(drone) # center the camera around the drone
    
    if update.ui_hovered
        tds.camera_update_by_user_input!(update.camera,
                                         update_obj.camera_motion_state,
                                         update.delta_time_us,
                                         update.delta_mouse_wheel,
                                         update.delta_mouse_pos,
                                         mode = tds.THIRD_PERSON)
    end
    
    return 0
end

viewport = tds.viewport_create()
update_sim_obj = Update_Sim()
tds.viewport_add_camera(viewport, camera, "drone camera", update, user_ptr=pointer_from_objref(update_sim_obj))

tds.gui_continue()
