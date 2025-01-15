#include "tiny_drones_render.hpp"

int main()
{
    RenderContextID ctxid = init_renderer(RenderConfig{.window_width=800, .window_height=600, .FPS=60, .background_color=WHITE});
    while (!renderer_should_close(ctxid)) {
        begin_3d_rendering(ctxid);
        render_sphere(ctxid, Sphere{{0, 0, 0}, BLACK, 0.1});
        render_sphere(ctxid, Sphere{{1, 1, 0}, BLACK, 0.1});
        render_grid_floor(ctxid, 60, 1.0);
        end_3d_rendering(ctxid);
    }
    close_renderer(ctxid);
}
