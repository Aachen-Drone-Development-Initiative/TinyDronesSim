using TinyDronesSim
const tds = TinyDronesSim

function update_camera(update::tds.Viewport_Camera_Callback_Update)::UInt32
    tds.viewport_activate(update.viewport)
    
    camera_motion_state = unsafe_pointer_to_objref(update.user_ptr)::tds.Camera_Motion_State
    if update.ui_hovered
        tds.camera_update_by_user_input!(update.camera,
                                         camera_motion_state,
                                         update.delta_time_us,
                                         update.delta_mouse_wheel,
                                         update.delta_mouse_pos)
    end
    return 0
end

# the env and viewport are automalically set as the active environment/viewport on creation
env = tds.env_create()
viewport = tds.viewport_create()

# creating a camera and adding it to the viewport.
# We also pass the callback 'update_camera' and a pointer to the 'camera_motion_state'.
# That pointer will be passed to our callback, where we recover the 'camera_motion_state' object.
# You can also just access global variables in the callback.
camera = tds.camera_create(env, far_plane = 200.0, pos = (1,2,3))
camera_motion_state = tds.Camera_Motion_State()
tds.viewport_add_camera(viewport, camera, "my camera", update_camera, user_ptr=pointer_from_objref(camera_motion_state))

tds.env_add_filamesh_from_file("./TinyDronesSimBackend/assets/suzanne.filamesh")
gltf_instance = tds.env_add_gltf_asset_and_create_instance("./TinyDronesSimBackend/assets/castle.glb")
gltf_instance_sibling = tds.gltf_inst_create_sibling(gltf_instance)
tds.gltf_inst_set_position(gltf_instance, (50, 0, 0))

tds.env_add_ibl_skybox("./TinyDronesSimBackend/assets/rogland_sunset_2k.hdr")

tds.gui_continue()
