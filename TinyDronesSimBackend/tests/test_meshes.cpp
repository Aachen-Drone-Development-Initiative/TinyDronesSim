#include "../tinydronessim_backend.hpp"
#include <math.hpp>
#include <cstdio>

double3 camera_pos = { 0, 1, 3 };
Camera_ID camera;

void camera_set_pos(void* user_ptr) {
    camera_pos = {-camera_pos.x, -camera_pos.y, -camera_pos.z};
    camera_set_position(camera, camera_pos);
}

int counter = 0;

uint32_t camera_update(Viewport_Frame_Callback_Update update)
{
    printf("cnt: %d\n", counter++);
    if (viewport_frame_key_down(update.vp_frame_id_idx, 'w')) {
        camera_pos.z -= (double) update.delta_time_us * 2e-5;
        printf("w\n");
    }
    if (viewport_frame_key_down(update.vp_frame_id_idx, 's')) {
        camera_pos.z += (double) update.delta_time_us * 2e-5;
        printf("s\n");
    }
    camera_set_position(camera, camera_pos);
    return 0;
}

int main()
{
    Environment_ID env = env_create();
    camera = camera_create(env);
    Viewport_ID viewport = viewport_create();

    camera_set_position(camera, camera_pos);

    env_add_lit_material("green_lit", {0.2, 0.9, 0.2}, 0.2);
    env_add_lit_material("blue_lit", {0.2, 0.2, 0.9}, 0.2);
    env_add_unlit_material("white_unlit");
    env_add_unlit_material("red_unlit", {1, 0, 0});
    env_add_unlit_material("green_unlit", {0, 1, 0});
    env_add_unlit_material("blue_unlit", {0, 0, 1});

    env_add_plane(2, 2, "green_lit");
    env_add_line({0, 0, 0}, {1, 0, 0}, "red_unlit");
    env_add_line({0, 0, 0}, {0, 1, 0}, "green_unlit");
    env_add_line({0, 0, 0}, {0, 0, 1}, "blue_unlit");
    Entity_ID cylinder = env_add_cylinder(1.5, 0.5, "green_lit");
    entity_set_position(cylinder, {0, 2, 0});
    
    [[maybe_unused]] Entity_ID arrow = env_add_arrow(0.1, 0.5, "blue_lit");
    // entity_set_position(arrow, {0, 2, 0});

    // Entity_ID sun = env_add_sun_light(30000, {0, 100, 0});
    env_add_ibl_skybox("./assets/rogland_sunset_2k.hdr");

    viewport_add_camera(viewport, camera, {"", "", 1}, camera_update, nullptr);

    Viewport_ID_Ctrl_Panel_IDX controll = viewport_add_ctrl_panel(viewport, {"down", "", 0.1}, false);
    viewport_ctrl_add_button(controll, "change cam pos", camera_set_pos, nullptr);

    return gui_continue();
}
