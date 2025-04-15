#include "../environments.h"
// #include <math.hpp>

int main()
{
    UUID env = create_environment();
    UUID camera = create_camera(env);
    UUID window = create_window(camera, 60, "environment test");

    // "/home/yuzeni/projekte/TinyDronesSim/EnvironmentBackend/assets/FlightHelmet/FlightHelmet.gltf"

    UUID suzanne = add_filamesh_from_file(env, "/home/yuzeni/projekte/TinyDronesSim/EnvironmentBackend/assets/suzanne.filamesh");
    set_position_and_orientation(suzanne, {1, 1, 1}, quaternion_ccw_90_y());
    
    UUID gltf_instance = add_gltf_asset_and_create_instance(env, "/home/yuzeni/projekte/TinyDronesSim/EnvironmentBackend/assets/TinyDroneEspS3.glb");

    set_position(gltf_instance, {-1, -2, -3});
    
    [[maybe_unused]] UUID gltf_instance_sib1 = create_gltf_instance_sibling(gltf_instance);

    add_lit_material(env, "base_lit");
    
    [[maybe_unused]] UUID plane = add_plane(env, {1, 1, 1}, 100, 100, "base_lit");
    
    add_ibl_skybox(env, "/home/yuzeni/projekte/TinyDronesSim/EnvironmentBackend/assets/rogland_sunset_2k.hdr");
    
    while(!window_should_close(window)) {
        update_window(window);
    }

    destroy_everything();
    
    return 0;
}
