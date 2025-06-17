#include "../tinydronessim_backend.hpp"
#include <cstdio>

int counter = 0;

uint32_t callback(Viewport_Camera_Callback_Update update)
{
    printf("counter: %d\n", counter);
    counter++;
    return 0;
}

int main()
{
    Environment_ID env = env_create();
    
    Camera_ID camera1 = camera_create(env);
    Camera_ID camera2 = camera_create(env);
    camera_set_position(camera2, {0, 0, 3});
    
    Entity_ID suzanne = env_add_filamesh_from_file("./assets/suzanne.filamesh");
    env_add_ibl_skybox("./assets/rogland_sunset_2k.hdr");

    Viewport_ID viewport = viewport_create();
    viewport_add_camera(viewport, camera1, callback, nullptr, "", "");
    // viewport_add_camera(viewport, camera2, callback, nullptr, "", "sus\tsussier");
    viewport_add_camera(viewport, camera2, callback, nullptr, "right", "");
    viewport_add_controll_panel(viewport, false, "", "cameras\tlol");

    return gui_continue();
}
