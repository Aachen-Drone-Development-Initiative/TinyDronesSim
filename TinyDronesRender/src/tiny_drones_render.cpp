#include "tiny_drones_render.hpp"

#include <cstdint>
#include <iostream>

struct {
    RenderContext* arr[MAX_RENDER_CONTEXTS - 1];
    int count = 0;
    
    RenderContext* create(RenderContext ctx)
    {
        arr[count] = new RenderContext{ctx};
        count++;
        return arr[count - 1];
    }
    RenderContext* operator[](RenderContextID ctxid) { return arr[ctxid.id]; }
} render_contexts;


RenderContextID init_renderer(RenderConfig cfg)
{
    RenderContext* ctx = render_contexts.create({.FPS=cfg.FPS,
            .background_color=cfg.background_color,
            .camera_mode=CAMERA_FREE});

    SetTraceLogLevel(LOG_ERROR);
    SetConfigFlags(FLAG_WINDOW_RESIZABLE);
    
    InitWindow(cfg.window_width, cfg.window_height, "Tiny Drones Renderer");
    
    ctx->camera = Camera{0};
    ctx->camera.position = (Vector3){ 0.0f, 10.0f, 10.0f };  // Camera position
    ctx->camera.target = (Vector3){ 0.0f, 0.0f, 0.0f };      // Camera looking at point
    ctx->camera.up = (Vector3){ 0.0f, 1.0f, 0.0f };          // Camera up vector (rotation towards target)
    ctx->camera.fovy = 45.0f;                                // Camera field-of-view Y
    ctx->camera.projection = CAMERA_PERSPECTIVE;             // Camera mode type

    SetTargetFPS(ctx->FPS);

    return RenderContextID{0}; // raylib only supports a single "context" all this context stuff is just for the future
}

bool renderer_should_close(RenderContextID ctxid)
{
    return WindowShouldClose();
}

static void update_renderer(RenderContext* ctx)
{
    static bool cursor_hidden = false;
    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        if (IsCursorOnScreen() && !cursor_hidden) {
            HideCursor();
            DisableCursor();
            cursor_hidden = true;
        }
        else if (cursor_hidden) {
            EnableCursor();
            ShowCursor();
            cursor_hidden = false;
        }
    }
    if (cursor_hidden) {
        UpdateCamera(&ctx->camera, ctx->camera_mode);
    }
}

void begin_3d_rendering(RenderContextID ctxid)
{
    RenderContext* ctx = render_contexts[ctxid];
    update_renderer(ctx);
    
    BeginDrawing();
    ClearBackground(ctx->background_color);
    BeginMode3D(ctx->camera);
}

void end_3d_rendering(RenderContextID ctxid)
{
    EndMode3D();
    EndDrawing();
}

void close_renderer(RenderContextID ctxid)
{
    CloseWindow();
}

void render_sphere(RenderContextID ctxid, Sphere sphere)
{
    // in raylib y is up (not z)
    DrawSphere(Vector3{(float)sphere.x[0], (float)sphere.x[2], (float)sphere.x[1]}, (float)sphere.radius, sphere.color);
}

void render_grid_floor(RenderContextID ctxid, int32_t n_fields, double field_spacing)
{
    DrawGrid(n_fields, field_spacing);
}
