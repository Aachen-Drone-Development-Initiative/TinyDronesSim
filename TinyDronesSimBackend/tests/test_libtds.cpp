#include "../tinydronessim_backend.hpp"
#include <math.hpp>
#include <iostream>

uint32_t callback1(Viewport_Camera_Callback_Update update)
{
    printf("delta mouse wheel: %d\n", update.delta_mouse_wheel);
    if (update.ui_hovered) {
        if (viewport_key_down('s')) {
            printf("cam1 s\n");
        }
        if (viewport_mouse_button_down(MOUSE_BUTTON_LEFT)) {
            int2 mouse_pos = viewport_mouse_pos();
            printf("mouse x: %d, y: %d\n", mouse_pos.x, mouse_pos.y);
        }
    }
    return 0;
}

uint32_t callback2(Viewport_Camera_Callback_Update update)
{
    printf("delta mouse pos. x: %d, y: %d\n", update.delta_mouse_pos.x, update.delta_mouse_pos.y);
    if (update.ui_hovered) {
        if (viewport_key_down('s')) {
            printf("cam2 s\n");
        }
        if (viewport_mouse_button_down(MOUSE_BUTTON_RIGHT)) {
            int64_t scroll_pos = viewport_mouse_scroll_pos();
            printf("mouse scroll: %ld\n", scroll_pos);
        }
    }
    return 0;
}

int main()
{
    Environment_ID env = env_create();
    
    Camera_ID camera1 = camera_create(env);
    Camera_ID camera2 = camera_create(env);
    camera_set_position(camera2, {0, 0, 3});
    
    Entity_ID suzanne = env_add_filamesh_from_file("./assets/suzanne.filamesh");
    entity_set_position_and_orientation(suzanne, {1, 1, 1}, quat_ccw_90_y());
    env_add_ibl_skybox("./assets/rogland_sunset_2k.hdr");

    // Viewport_ID viewport = viewport_create(60, callback, "ud(0.9, lr(0.7, tabs('camera 1', 'camera 2'))), 'ControllPanel')");
    Viewport_ID viewport = viewport_create();
    
    viewport_add_camera(viewport, camera1, "camera 1", callback1, nullptr);
    viewport_add_camera(viewport, camera2, "camera 2", callback2, nullptr);

    glTF_Instance_ID gltf_inst = env_add_gltf_asset_and_create_instance("./assets/TinyDroneEspS3.glb");
    gltf_inst_set_position(gltf_inst, {-1, -2, -3});
    
    [[maybe_unused]] glTF_Instance_ID gltf_instance_sib1 = gltf_inst_create_sibling(gltf_inst);

    env_add_lit_material("base_lit");
    
    [[maybe_unused]] Entity_ID plane = env_add_plane({1, 1, 1}, 100, 100, "base_lit", quat_identity());

    for (int i = 0; i < 2; ++i) {
        gui_continue();
    }
    
    destroy_everything();

    return 0;
}
