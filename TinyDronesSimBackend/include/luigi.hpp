#pragma once
#include "../tinydronessim_backend.hpp"

#define UI_LINUX // we need this, to get the UI_FREE macro
#include <luigi2.h>

#include <chrono>

void init_luigi_if_not_init();

typedef struct UIViewportFramePixels {
    size_t width, height;
    size_t pixel_alignment;
    uint8_t* bits;
} UIViweportFramePixels;

typedef struct UIViewportFrame {
#define UI_VIEWPORT_FRAME_PARENT_MANAGED_WINDOW_MODE (1 << 0) // Create and manage a new window for totally independent drawing
    UIElement e;
    uint32_t *bits;
    int width, height;
    float zoom;
    bool visible;

    UIWindow* window; // exclusively for UI_VIEWPORT_FRAME_PARENT_MANAGED_WINDOW_MODE

    void *callback_state;
    // Width and height are just a suggestion. (Size of the visible portion of the frame).
    UIViewportFramePixels (*get_new_pixels)(void* callback_state, size_t width, size_t height);
    void (*render_to_managed_window)(void* callback_state, UIWindow* window, size_t width, size_t height);
} UIViewportFrame;

UIViewportFrame *UIViewportFrameCreate(UIElement *parent, uint32_t flags, uint32_t width, uint32_t height);
/* void UIViewportFrameSetContent(UIViewportFrame *vp_frame, uint8_t *bits, size_t width, size_t height, size_t pixel_alignment); */
void UIViewportFrameSetZoomFactor(UIViewportFrame *vp_frame, float zoom);

struct UIAnimateTempo {
    UIElement e;
    
    std::chrono::high_resolution_clock::time_point prev_time_point;
    uint64_t dt_micro_s = 0;
    uint32_t target_fps;
};

UIAnimateTempo *UIAnimateTempoCreate(UIElement *parent, uint32_t flags, uint32_t target_fps);
