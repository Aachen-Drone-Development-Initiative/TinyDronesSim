using TinyDronesSim
const TDS = TinyDronesSim
const ENV = TinyDronesSim.Environments

env = ENV.create_environment()
camera = ENV.create_camera(env, ENV.Camera_Settings())
camera_motion_state = ENV.Camera_Motion_State()
camera_motion_settings = ENV.Camera_Motion_Settings()
window = ENV.create_window(camera, target_fps = 60)

ENV.add_filamesh_from_file(env, cstatic"/home/yuzeni/projekte/TinyDronesSim/EnvironmentBackend/assets/suzanne.filamesh")

gltf_instance = ENV.add_gltf_asset_and_create_instance(env, cstatic"/home/yuzeni/projekte/TinyDronesSim/EnvironmentBackend/assets/magic_laboratory.glb");

gltf_instance_entity = ENV.get_gltf_instance_entity(env, gltf_instance)
ENV.set_position(env, gltf_instance_entity, Float64_3(-1, -2, -3))

gltf_instance_sib1 = ENV.create_gltf_instance_sibling(env, gltf_instance)

ENV.add_ibl_skybox(env, cstatic"/home/yuzeni/projekte/TinyDronesSim/EnvironmentBackend/assets/rogland_sunset_2k.hdr")

try
    while !ENV.window_should_close(window)
        ENV.update_window(window)
        ENV.update_camera_by_window_input!(camera, window, camera_motion_state, camera_motion_settings)
    end
finally
    # destroy in revese order of creation
    ENV.destroy_window!(window)
    ENV.destroy_camera!(camera)
    ENV.destroy_environment!(env)
end
