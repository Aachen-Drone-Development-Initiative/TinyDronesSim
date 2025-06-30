#pragma once
#include "../tinydronessim_backend.hpp"

#define UI_LINUX // we need this, to get the UI_FREE macro
#include <luigi2.h>

#include <vector>

void init_luigi_if_not_init();

struct UIViewportFramePixels {
    size_t width, height;
    size_t pixel_alignment;
    uint8_t* bits;
};

struct Viewport_Frame;

struct UIViewportFrame {
#define UI_VIEWPORT_FRAME_PARENT_MANAGED_WINDOW_MODE (1 << 0) // Create and manage a new window for totally independent drawing
    UIElement e;
    uint32_t *bits;
    int width, height;
    float zoom;
    bool visible;

    UIWindow* window; // exclusively for UI_VIEWPORT_FRAME_PARENT_MANAGED_WINDOW_MODE

    // Width and height are just a suggestion. (Size of the visible portion of the frame).
    UIViewportFramePixels (*get_new_pixels)(Viewport_Frame* vp_frame, size_t width, size_t height);
    void (*render_to_managed_window)(Viewport_Frame* vp_frame, UIWindow* window, size_t width, size_t height);
    
    Viewport_Frame* vp_frame;
};

UIViewportFrame *UIViewportFrameCreate(UIElement *parent, uint32_t flags, uint32_t width, uint32_t height, Viewport_Frame* vp_frame);
/* void UIViewportFrameSetContent(UIViewportFrame *vp_frame, uint8_t *bits, size_t width, size_t height, size_t pixel_alignment); */
void UIViewportFrameSetZoomFactor(UIViewportFrame* vp_frame, float zoom);

struct Viewport_Plot;

struct UIViewportPlot {
#define UI_VIEWPORT_PLOT_INTERACTIVE (1 << 0)
    UIElement e;
    // Axis Scale and Position
    double x_begin, y_begin;
    double x_end, y_end;
    
    uint32_t max_length; // 0 means unbounded, used for first in last out plotting
    Viewport_Plot* vp_plot;
};

UIViewportPlot *UIViewportPlotCreate(UIElement *parent, uint32_t flags);
