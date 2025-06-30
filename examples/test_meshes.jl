using TinyDronesSim
const tds = TinyDronesSim

env = tds.env_create()
camera = tds.camera_create(env)
viewport = tds.viewport_create()

tds.env_add_lit_material("green_lit", color = [0.2, 0.9, 0.2], roughness = 0.2)
tds.env_add_lit_material("blue_lit", color = [0.2, 0.2, 0.9], roughness = 0.2)
tds.env_add_unlit_material("white_unlit")
tds.env_add_unlit_material("red_unlit", color = [1, 0, 0])
tds.env_add_unlit_material("green_unlit", color = [0, 1, 0])
tds.env_add_unlit_material("blue_unlit", color = [0, 0, 1])

tds.env_add_plane(2, 2, "green_lit")
tds.env_add_line([0, 0, 0], [1, 0, 0], "red_unlit")
tds.env_add_line([0, 0, 0], [0, 1, 0], "green_unlit")
tds.env_add_line([0, 0, 0], [0, 0, 1], "blue_unlit")
cylinder = tds.env_add_cylinder(1.5, 0.5, "green_lit")
tds.entity_set_position(cylinder, [0, 2, 0])

arrow = tds.env_add_arrow(0.01, 0.05, "blue_lit")
# entity_set_position(arrow, [0, 2, 0])

# sun = env_add_sun_light(30000, [0, 100, 0])
tds.env_add_ibl_skybox("./TinyDronesSimBackend/assets/rogland_sunset_2k.hdr")

camera_motion_state = tds.Camera_Motion_State()
function camera_update(update::tds.Viewport_Frame_Callback_Update)::UInt32
    tds.viewport_frame_update_camera_by_user_input!(update.viewport_frame,
                                                    update.camera,
                                                    camera_motion_state,
                                                    update.delta_time_us,
                                                    update.delta_mouse_wheel,
                                                    update.delta_mouse_drag)
    return 0
end

tds.viewport_add_camera(viewport, camera, user_callback = camera_update)

plot = tds.viewport_add_plot(viewport, location_path = "right", weight = 0.5)

tds.gui_continue()
