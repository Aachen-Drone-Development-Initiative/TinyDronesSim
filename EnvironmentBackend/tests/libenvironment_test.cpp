#include "../environments.hpp"
#include <iostream>

int main()
{
    Environment_ID env = create_environment();
    std::cout << "env id: " << env.id << '\n';
    Camera_ID camera = create_camera(env);
    Camera_ID second_camera = create_camera(env);
    
    Window_ID window = create_window(camera, 60, "environment test");
    Window_ID second_window = create_window(second_camera, 60, "environment test second window");

    // "/home/yuzeni/projekte/TinyDronesSim/EnvironmentBackend/assets/FlightHelmet/FlightHelmet.gltf"

    Filament_Entity_ID suzanne = add_filamesh_from_file("./assets/suzanne.filamesh");
    set_position_and_orientation(suzanne, {1, 1, 1}, quaternion_ccw_90_y());
    
    glTF_Instance_ID gltf_instance = add_gltf_asset_and_create_instance("./assets/TinyDroneEspS3.glb");

    set_position(get_gltf_instance_filament_entity(gltf_instance), {-1, -2, -3});
    
    [[maybe_unused]] glTF_Instance_ID gltf_instance_sib1 = create_gltf_instance_sibling(gltf_instance);

    add_lit_material("base_lit");
    
    [[maybe_unused]] Filament_Entity_ID plane = add_plane({1, 1, 1}, 100, 100, "base_lit");
    
    add_ibl_skybox("./assets/rogland_sunset_2k.hdr");

    while(window_visible(window) || window_visible(second_window))
    {
        if (window_visible(window)) {
            window_activate(window);
            
            if (is_key_pressed(SDL_SCANCODE_SPACE)) {
                std::cout << "trying to connect: " << connect_to_joystick() << '\n';
            }
            if (is_connected_to_joystick()) {
                std::cout << "axis 0: " << get_joystick_axis_raw(0) << '\n';
            }
            window_update();
        }
        
        if (window_visible(second_window)) {
            window_activate(second_window);
            window_update();
        }
    }

    destroy_everything();
    
    return 0;
}
