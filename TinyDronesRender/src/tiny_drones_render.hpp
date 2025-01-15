#include "../raylib/raylib.h"
#include <cstdint>

#define MAX_RENDER_CONTEXTS 128

struct RenderContextID {
    int64_t id;
};

struct RenderContext {
    int32_t FPS;
    Color background_color;
    Camera3D camera;
    CameraMode camera_mode;
};

struct RenderConfig {
    int32_t window_width;
    int32_t window_height;
    int32_t FPS;
    Color background_color;
};

extern "C" {
    RenderContextID init_renderer(RenderConfig cfg);
    bool renderer_should_close(RenderContextID ctxid);
    void begin_3d_rendering(RenderContextID ctxid);
    void end_3d_rendering(RenderContextID ctxid);
    void close_renderer(RenderContextID ctxid);

    void render_grid_floor(RenderContextID ctxid, int32_t n_fields, double field_spacing);
}

struct Sphere {
    double x[3];
    Color color;
    double radius;
};

extern "C" void render_sphere(RenderContextID ctxid, Sphere sphere);


