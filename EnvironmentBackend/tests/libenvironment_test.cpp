#include "../environments.h"
#include <iostream>

int main()
{
    UUID env = create_environment();
    UUID camera = create_camera(env);
    UUID second_camera = create_camera(env);
    
    UUID window = create_window(camera, 60, "environment test");
    UUID second_window = create_window(second_camera, 60, "environment test second window");

    // "/home/yuzeni/projekte/TinyDronesSim/EnvironmentBackend/assets/FlightHelmet/FlightHelmet.gltf"

    UUID suzanne = add_filamesh_from_file("./assets/suzanne.filamesh");
    set_position_and_orientation(suzanne, {1, 1, 1}, quaternion_ccw_90_y());
    
    UUID gltf_instance = add_gltf_asset_and_create_instance("./assets/TinyDroneEspS3.glb");

    set_position(gltf_instance, {-1, -2, -3});
    
    [[maybe_unused]] UUID gltf_instance_sib1 = create_gltf_instance_sibling(gltf_instance);

    add_lit_material("base_lit");
    
    [[maybe_unused]] UUID plane = add_plane({1, 1, 1}, 100, 100, "base_lit");
    
    add_ibl_skybox("./assets/rogland_sunset_2k.hdr");

    while(exists(window) || exists(second_window))
    {
        if (exists(window)) {
            set_active_window(window);
            
            if (is_key_pressed(SDL_SCANCODE_SPACE)) {
                std::cout << "trying to connect: " << connect_to_joystick() << '\n';
            }
            if (is_connected_to_joystick()) {
                std::cout << "axis 0: " << get_joystick_axis_raw(0) << '\n';
            }
            update_window();
        }

        if (exists(second_window)) {
            set_active_window(second_window);
            update_window();
        }
    }

    destroy_everything();
    
    return 0;
}
