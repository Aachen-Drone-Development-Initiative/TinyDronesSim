#include "../tinydronessim_backend.hpp"
#include <cstdio>

int counter = 0;

uint32_t callback(Viewport_Frame_Callback_Update update)
{
    // printf("counter: %d\n", counter);
    counter++;
    return 0;
}

void clear_button_callback(void* user_ptr) {
    Viewport_ID_Plot_IDX plot = *((Viewport_ID_Plot_IDX*) user_ptr);
    viewport_plot_clear(plot);
}

void restrict_button_callback(void* user_ptr) {
    Viewport_ID_Plot_IDX plot = *((Viewport_ID_Plot_IDX*) user_ptr);
    viewport_plot_restrict_length(plot, 200);
}

void unrestrict_button_callback(void* user_ptr) {
    Viewport_ID_Plot_IDX plot = *((Viewport_ID_Plot_IDX*) user_ptr);
    viewport_plot_restrict_length(plot, 0);
}

void slider_callback(double value, void* user_ptr) {
    Viewport_ID_Plot_IDX plot = *((Viewport_ID_Plot_IDX*) user_ptr);
    viewport_plot_append_number(plot, value);
}

int main()
{
    Environment_ID env = env_create();
    
    Camera_ID camera1 = camera_create(env);
    Camera_ID camera2 = camera_create(env);
    camera_set_position(camera2, {0, 0, 3});
    
    [[maybe_unused]] Entity_ID suzanne = env_add_filamesh_from_file("./assets/suzanne.filamesh");
    env_add_ibl_skybox("./assets/rogland_sunset_2k.hdr");

    Viewport_ID viewport = viewport_create();
    
    Viewport_ID_Plot_IDX plot = viewport_add_plot(viewport, {"", "", 0.5});
    double numbers[] = { 1, 4, 3, 5,4, -2, -1, 4 };
    viewport_plot_set_numbers(plot, numbers, sizeof(numbers) / sizeof(double));
    viewport_plot_restrict_length(plot, 200);
    
    Viewport_ID_Ctrl_Panel_IDX controll = viewport_add_ctrl_panel(viewport, {"down", "controll", 0.1}, false);
    viewport_ctrl_add_button(controll, "clear plot", clear_button_callback, &plot);
    viewport_ctrl_add_button(controll, "show latest", restrict_button_callback, &plot);
    viewport_ctrl_add_button(controll, "show all", unrestrict_button_callback, &plot);
    viewport_ctrl_add_slider(controll, "plotted slider", slider_callback, &plot, 0, -10, 10, 5);
    
    viewport_add_camera(viewport, camera1, {"", "plotting|cameras", 1}, callback, nullptr);
    viewport_add_camera(viewport, camera2, {"cameras", "camera1|camera2", 1}, callback, nullptr);

    return gui_continue();
}
