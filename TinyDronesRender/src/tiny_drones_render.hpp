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

struct Vec3f_32 {
    float a[3];
};

extern "C" {
    RenderContextID init_renderer(RenderConfig cfg);
    bool renderer_should_close(RenderContextID ctxid);
    void begin_3d_rendering(RenderContextID ctxid);
    void end_3d_rendering(RenderContextID ctxid);
    void close_renderer(RenderContextID ctxid);

    void render_grid_floor(RenderContextID ctxid, int n_fields, float field_spacing);
    void render_sphere(RenderContextID ctxid, Vec3f_32 pos, float radius, Color color);
    void render_path(RenderContextID ctxid, Vec3f_32* points, int n_points, Color color);
}

