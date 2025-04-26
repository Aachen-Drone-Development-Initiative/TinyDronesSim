using TinyDronesSim
using StaticStrings
const Env = TinyDronesSim.Environments

env = Env.create_environment()
camera = Env.create_camera(env, far_plane = 200.0)
camera_motion_state = Env.Camera_Motion_State()
window = Env.create_window(camera, cstatic"environment test")

Env.add_filamesh_from_file(cstatic"./EnvironmentBackend/assets/suzanne.filamesh")

gltf_instance = Env.add_gltf_asset_and_create_instance(cstatic"./EnvironmentBackend/assets/castle.glb");
gltf_instance_sibling = Env.create_gltf_instance_sibling(gltf_instance)
Env.set_position(gltf_instance, Float64_3(50, 0, 0))

Env.add_ibl_skybox(cstatic"./EnvironmentBackend/assets/rogland_sunset_2k.hdr")

try
    while Env.exists(window)
        Env.update_camera_by_window_input!(camera, camera_motion_state)
        Env.update_window()
    end
finally
    Env.destroy_everything()
end
