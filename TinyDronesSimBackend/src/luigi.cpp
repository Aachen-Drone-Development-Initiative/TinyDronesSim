

#include "math.hpp"
#include <cmath>
#include <cstddef>
#include <cstdint>
#define UI_LINUX
// #define UI_DEBUG
#define UI_IMPLEMENTATION
#include <luigi.hpp>

#include <object_manager.hpp>
#include <viewport.hpp>
#include <logging.hpp>

#include <thread>

void init_luigi_if_not_init()
{
    static bool initialized = false;
    if (initialized) return;

    UIInitialise();
    ui.theme = uiThemeDark;
    initialized = true;
}

TDS_API int gui_continue()
{
    auto viewports = g_objm.get_viewports();
    for (std::pair<Viewport_ID, Viewport*> viewport_entry : viewports) {
        viewport_entry.second->show();
    }

    return UIMessageLoop();
}

/////////////////////////////////////////
// Viewport Frame.
/////////////////////////////////////////

int _UIViewportFrameMessage(UIElement *element, UIMessage message, int di, void *dp) {
    UIViewportFrame *vp_frame = (UIViewportFrame *) element;

    if (message == UI_MSG_GET_HEIGHT) {
        return vp_frame->height * vp_frame->zoom;
    } else if (message == UI_MSG_GET_WIDTH) {
        return vp_frame->width * vp_frame->zoom;
    } else if (message == UI_MSG_DEALLOCATE) {
        UI_FREE(vp_frame->bits);
    } else if (message == UI_MSG_PAINT) {
        UIPainter *painter = (UIPainter *) dp;

        int x = element->bounds.l;
        int y = element->bounds.t;
        
        UIRectangle image = UI_RECT_4(x, x + (int) vp_frame->width, y, y + (int) vp_frame->height);
        UIRectangle bounds;
        
        // When we are in UI_VIEWPORT_FRAME_PARENT_MANAGED_WINDOW_MODE we can paint without a painter
        // so its not always necessary.
        if (painter) {
            bounds = UIRectangleIntersection(painter->clip, UIRectangleIntersection(vp_frame->e.bounds, image));
        }
        else {
            tds_assert(vp_frame->e.flags & UI_VIEWPORT_FRAME_PARENT_MANAGED_WINDOW_MODE);
            bounds = UIRectangleIntersection(vp_frame->e.bounds, image);
        }
        
        if (!UI_RECT_VALID(bounds)) return 0;

        if (vp_frame->e.flags & UI_VIEWPORT_FRAME_PARENT_MANAGED_WINDOW_MODE) {
            UI_ASSERT(vp_frame->render_to_managed_window);
            UIWindowMoveResize(vp_frame->window, bounds.l, bounds.t, UI_RECT_WIDTH(element->bounds), UI_RECT_HEIGHT(element->bounds));
            // XSync would ensure that the glx/egl can render to a valid window, with resize completed etc.
            // Its not enabeled, because this introduces serious lag. The consequence is flickering and other artifacts during resize.
            // If the image is not rendered continously, it can also stay black after resize.
            // XSync(ui.display, true);
            vp_frame->render_to_managed_window(vp_frame->vp_frame, vp_frame->window, UI_RECT_WIDTH(element->bounds), UI_RECT_HEIGHT(element->bounds));
        }
        else {
            UI_ASSERT(vp_frame->get_new_pixels);
            UIViewportFramePixels pixels = vp_frame->get_new_pixels(vp_frame->vp_frame, UI_RECT_WIDTH(bounds), UI_RECT_HEIGHT(bounds));

            // Updating the frame with new pixels.
            size_t old_size = vp_frame->width * vp_frame->height;
            size_t new_size = pixels.width * pixels.height;
            vp_frame->width = pixels.width;
            vp_frame->height = pixels.height;
    
            if (old_size != new_size) {
                // TODO: At least change this to UI_FREE, UI_MALLOC
                free(vp_frame->bits);
                vp_frame->bits = (uint32_t*) malloc(new_size * sizeof(uint32_t));
                /* UI_REALLOC(vp_frame->bits, new_size * sizeof(uint32_t)); */
            }
    
            for (size_t i = 0; i < pixels.height; ++i) {
                for (size_t j = 0; j < pixels.width; ++j) {
                    uint32_t color = 0;
                    color += (uint32_t)pixels.bits[i * (pixels.width * pixels.pixel_alignment) + j * pixels.pixel_alignment + 2]; // Red
                    color += (uint32_t)pixels.bits[i * (pixels.width * pixels.pixel_alignment) + j * pixels.pixel_alignment + 1] << (8 * 1); // Green
                    color += (uint32_t)pixels.bits[i * (pixels.width * pixels.pixel_alignment) + j * pixels.pixel_alignment]     << (8 * 2); // Blue
                    color += (uint32_t)0xff                                                                                      << (8 * 3); // Alpha
                    vp_frame->bits[i * pixels.width + j] = color;
                }
            }

            // Recalculating image and bounds rectangles.
            image = UI_RECT_4(x, x + (int) vp_frame->width, y, y + (int) vp_frame->height);
            bounds = UIRectangleIntersection(painter->clip, UIRectangleIntersection(vp_frame->e.bounds, image));

            /* if (vp_frame->zoom == 1) { */
            uint32_t *lineStart = (uint32_t *) painter->bits + bounds.t * painter->width + bounds.l;
            uint32_t *sourceLineStart = vp_frame->bits + (bounds.l - image.l) + vp_frame->width * (bounds.t - image.t);
    
            for (int i = 0; i < bounds.b - bounds.t; i++, lineStart += painter->width, sourceLineStart += vp_frame->width) {
                uint32_t *destination = lineStart;
                uint32_t *source = sourceLineStart;
                int j = bounds.r - bounds.l;
    
                do {
                    *destination = *source;
                    destination++;
                    source++;
                } while (--j);
            }
            /* } else { */
            /*     float zr = 1.0f / vp_frame->zoom; */
            /*     uint32_t *destination = (uint32_t *) painter->bits; */
            
            /*     for (int i = bounds.t; i < bounds.b; i++) { */
            /*         int ty = (i - image.t) * zr; */
                
            /*         for (int j = bounds.l; j < bounds.r; j++) { */
            /*             int tx = (j - image.l) * zr; */
            /*             destination[i * painter->width + j] = vp_frame->bits[ty * vp_frame->width + tx]; */
            /*         } */
            /*     } */
            /* } */
        }
    } else if (message == UI_MSG_ANIMATE) {
        if (vp_frame->visible) {
            if (vp_frame->e.flags & UI_VIEWPORT_FRAME_PARENT_MANAGED_WINDOW_MODE) {
                // if we render manually to a subwindow (with glx or egl, ie. swap-chain stuff)
                // We can just paint directly and pass 'nullptr' instead of a valid 'painter'
                // This improves performance, since 'UIElementRepaint()' would only 'shedule'
                // the painting for the next time '_UIUpdate()' is called.
                // Instead this happens instantly.
                UIElementMessage(element, UI_MSG_PAINT, 0, nullptr);
            }
            else {
                UIElementRepaint(element, nullptr);
            }
        }
    } else if (message == UI_MSG_TAB_SELECTED) {
        vp_frame->visible = true;
        if (vp_frame->e.flags & UI_VIEWPORT_FRAME_PARENT_MANAGED_WINDOW_MODE) {
            XMapWindow(ui.display, vp_frame->window->window);
            XRaiseWindow(ui.display, vp_frame->window->window);
        }
        UIElementFocus(element);
    } else if (message == UI_MSG_TAB_OTHER_SELECTED) {
        vp_frame->visible = false;
        if (vp_frame->e.flags & UI_VIEWPORT_FRAME_PARENT_MANAGED_WINDOW_MODE) {
            XUnmapWindow(ui.display, vp_frame->window->window);
        }
    } else if (message == UI_MSG_LEFT_DOWN) {
        vp_frame->vp_frame->m_input.mouse_left_down = true;
        UIElementFocus(element);
    } else if (message == UI_MSG_LEFT_UP) {
        vp_frame->vp_frame->m_input.mouse_left_down = false;
        vp_frame->vp_frame->m_input.mouse_is_dragging = false;
    } else if (message == UI_MSG_RIGHT_DOWN) {
        vp_frame->vp_frame->m_input.mouse_right_down = true;
    } else if (message == UI_MSG_RIGHT_UP) {
        vp_frame->vp_frame->m_input.mouse_right_down = false;
        vp_frame->vp_frame->m_input.mouse_is_dragging = false;
    } else if (message == UI_MSG_MIDDLE_DOWN) {
        vp_frame->vp_frame->m_input.mouse_middle_down = true;
    } else if (message == UI_MSG_MIDDLE_UP) {
        vp_frame->vp_frame->m_input.mouse_middle_down = false;
        vp_frame->vp_frame->m_input.mouse_is_dragging = false;
    } else if (message == UI_MSG_MOUSE_DRAG) {
        vp_frame->vp_frame->m_input.mouse_position = { element->window->cursorX, element->window->cursorY };
        vp_frame->vp_frame->m_input.mouse_is_dragging = true;
    } else if (message == UI_MSG_MOUSE_WHEEL) {
        vp_frame->vp_frame->m_input.mouse_scroll_position += di;
        return 1;
    } else if (message == UI_MSG_KEY_TYPED) {
        UIKeyTyped* key = (UIKeyTyped*) dp;
        if (key->code >= 0 && (size_t)key->code < 256) {
            vp_frame->vp_frame->m_input.ascii_key_down[key->code] = true;
        }
        return 0;
    }
    else if (message == UI_MSG_KEY_RELEASED) {
        UIKeyTyped* key = (UIKeyTyped*) dp;
        if (key->code >= 0 && (size_t)key->code < 256) {
            vp_frame->vp_frame->m_input.ascii_key_down[key->code] = false;
        }
        return 0;
    }
    
    return 0;
}

void UIViewportFrameSetZoomFactor(UIViewportFrame *vp_frame, float zoom)
{
    vp_frame->zoom = zoom;
    UIElementMeasurementsChanged(&vp_frame->e, 3);
    UIElementRepaint(&vp_frame->e, NULL);
}

/* void UIViewportFrameSetContent(UIViewportFrame *vp_frame, uint8_t *bits, size_t width, size_t height, size_t pixel_alignment) { */
/*     UI_ASSERT(pixel_alignment == 3); // We expect RGB only (no Alpha) */
/*     UI_ASSERT(~vp_frame->e.flags & UI_VIEWPORT_FRAME_PARENT_MANAGED_WINDOW_MODE); // Dont set content if rendering is extern */
    
/*     size_t old_size = vp_frame->width * vp_frame->height; */
/*     size_t new_size = width * height; */
/*     vp_frame->width = width; */
/*     vp_frame->height = height; */
    
/*     if (old_size != new_size) { */
/*         // TODO: At least change this to UI_FREE, UI_MALLOC */
/*         free(vp_frame->bits); */
/*         vp_frame->bits = (uint32_t*) malloc(new_size * sizeof(uint32_t)); */
/*         /\* UI_REALLOC(vp_frame->bits, new_size * sizeof(uint32_t)); *\/ */
/*     } */
    
/*     for (int i = 0; i < height; ++i) { */
/*         for (int j = 0; j < width; ++j) { */
/*             uint32_t color = 0; */
/*             color += (uint32_t)bits[i * (width * pixel_alignment) + j * pixel_alignment + 2]; // Red */
/*             color += (uint32_t)bits[i * (width * pixel_alignment) + j * pixel_alignment + 1] << (8 * 1); // Green */
/*             color += (uint32_t)bits[i * (width * pixel_alignment) + j * pixel_alignment]     << (8 * 2); // Blue */
/*             color += (uint32_t)0xff                                                          << (8 * 3); // Alpha */
/*             vp_frame->bits[i * vp_frame->width + j] = color; */
/*         } */
/*     } */

/*     UIElementMeasurementsChanged(&vp_frame->e, 3); */
/*     UIElementRepaint(&vp_frame->e, NULL); */
/* } */

UIViewportFrame *UIViewportFrameCreate(UIElement *parent, uint32_t flags, uint32_t width, uint32_t height, Viewport_Frame* vp_frame) {
    UIViewportFrame *ui_vp_frame = (UIViewportFrame *) UIElementCreate(sizeof(UIViewportFrame), parent, flags, _UIViewportFrameMessage, "Viewport Frame");
    ui_vp_frame->width = width;
    ui_vp_frame->height = height;
    ui_vp_frame->vp_frame = vp_frame;
    ui_vp_frame->zoom = 1.0f;
    ui_vp_frame->visible = true;
    
    if (ui_vp_frame->e.flags & UI_VIEWPORT_FRAME_PARENT_MANAGED_WINDOW_MODE) {
        ui_vp_frame->window = UIWindowCreate(parent->window, UI_WINDOW_PARENT_MANAGED_SUBWINDOW, "", 20, 20);
        /* XUnmapWindow(ui.display, ui_vp_frame->window->window); */
    }
    else {
        ui_vp_frame->bits = (uint32_t*) UI_MALLOC(width * height * sizeof(uint32_t));
    }
    
    return ui_vp_frame;
}

/////////////////////////////////////////
// Viewport Plot
/////////////////////////////////////////

int _UIViewportPlotMessage(UIElement *element, UIMessage message, int di, void *dp) {
    UIViewportPlot *ui_vp_plot = (UIViewportPlot *) element;

    if (message == UI_MSG_PAINT) {
        UIPainter *painter = (UIPainter *) dp;

        Viewport_Plot* plot = ui_vp_plot->vp_plot;

        UIRectangle bounds = UIRectangleIntersection(painter->clip, ui_vp_plot->e.bounds);
        if (!UI_RECT_VALID(bounds)) return 0;

        if (plot && !plot->empty()) {
            if (~ui_vp_plot->e.flags & UI_VIEWPORT_PLOT_INTERACTIVE) {
                // Fit the screen to the plot data.
                double x_min = INFINITY, y_max = -INFINITY; 
                double y_min = INFINITY, x_max = -INFINITY;
                
                size_t i = plot->get_start_idx();
                
                if (plot->has_x_values) {
                    for (; i < plot->points.size(); ++i) {
                        x_min = plot->points[i].x < x_min ? plot->points[i].x : x_min;
                        x_max = plot->points[i].x > x_max ? plot->points[i].x : x_max;
                        y_min = plot->points[i].y < y_min ? plot->points[i].y : y_min;
                        y_max = plot->points[i].y > y_max ? plot->points[i].y : y_max;
                    }
                }
                else {
                    x_min = i;
                    x_max = plot->points.size() > x_max ? plot->points.size() : x_max;
                    for (; i < plot->points.size(); ++i) {
                        y_min = plot->points[i].y < y_min ? plot->points[i].y : y_min;
                        y_max = plot->points[i].y > y_max ? plot->points[i].y : y_max;
                    }
                }
            
                ui_vp_plot->x_begin = x_min;
                ui_vp_plot->x_end = x_max;
                ui_vp_plot->y_begin = y_min;
                ui_vp_plot->y_end = y_max;
            }
        }

        // Drawing the "ticks" ie. the x/y grid-lines and x/y grid-labels

        const int tick_count_max = 20;
        const int x_pixels_per_tick = 100;
        const int y_pixels_per_tick = 50;
        int x_tick_count = std::min((int) floor(UI_RECT_WIDTH(bounds) / x_pixels_per_tick), tick_count_max);
        int y_tick_count = std::min((int) floor(UI_RECT_HEIGHT(bounds) / y_pixels_per_tick), tick_count_max);
        
        auto calculate_tick_spacing = [](double begin, double end, int tick_count) -> double {
            double raw_step = (end - begin) / tick_count;
            double exponent = floor(log10(raw_step));
            double base = pow(10, exponent);
            double fraction = raw_step / base;
            double nice_fractions[] = {1, 2, 2.5, 5, 10};
            double best_fraction = 1;
            for (size_t i = 0; i < sizeof(nice_fractions) / sizeof(double); ++i) {
                if (fraction <= nice_fractions[i]) {
                    best_fraction = nice_fractions[i];
                    break;
                }
            }
            return best_fraction * base;
        };

        double x_tick_spacing = calculate_tick_spacing(ui_vp_plot->x_begin, ui_vp_plot->x_end, x_tick_count);
        double x_tick_begin = ceil(ui_vp_plot->x_begin / x_tick_spacing) * x_tick_spacing;
        double y_tick_spacing = calculate_tick_spacing(ui_vp_plot->y_begin, ui_vp_plot->y_end, y_tick_count);
        double y_tick_begin = ceil(ui_vp_plot->y_begin / y_tick_spacing) * y_tick_spacing;

        const int max_tick_text_length = 16;
        char x_tick_text[max_tick_text_length][tick_count_max];
        char y_tick_text[max_tick_text_length][tick_count_max];
        int max_y_tick_text_width = 0;
        
        int tick_idx = 0;
        for (double x = x_tick_begin; x < ui_vp_plot->x_end && tick_idx < tick_count_max; x += x_tick_spacing, ++tick_idx) {
            snprintf(x_tick_text[tick_idx], max_tick_text_length, "%g", abs(x) > 1e-9 ? x : 0.0);
        }
        tick_idx = 0;
        for (double y = y_tick_begin; y < ui_vp_plot->y_end && tick_idx < tick_count_max; y += y_tick_spacing, ++tick_idx) {
            snprintf(y_tick_text[tick_idx], max_tick_text_length, "%g", abs(y) > 1e-9 ? y : 0.0);
            int tick_str_width = UIMeasureStringWidth(y_tick_text[tick_idx], -1);
            max_y_tick_text_width = tick_str_width > max_y_tick_text_width ? tick_str_width : max_y_tick_text_width;
        }

        int text_height = UIMeasureStringHeight();
        UIRectangle border = { max_y_tick_text_width + 5, 0, 0, text_height + 5};
        UIDrawRectangle(painter, bounds, ui.theme.panel2, ui.theme.panel1, border);

        UIRectangle plot_screen = { bounds.l + border.l, bounds.r - border.r, bounds.t + border.t, bounds.b - border.b };

        auto x_to_screenspace = [=](double x_world) -> int {
            return round(linear_map(x_world, ui_vp_plot->x_begin, ui_vp_plot->x_end, (double) plot_screen.l, (double) plot_screen.r));
        };
        auto y_to_screenspace = [=](double y_world) -> int {
            return round(linear_map(y_world, ui_vp_plot->y_begin, ui_vp_plot->y_end, (double) plot_screen.b, (double) plot_screen.t));
        };

        tick_idx = 0;
        for (double x = x_tick_begin; x < ui_vp_plot->x_end; x += x_tick_spacing, ++tick_idx) {
            UIDrawLine(painter, x_to_screenspace(x), plot_screen.t, x_to_screenspace(x), plot_screen.b, ui.theme.panel1);
            UIDrawString(painter, UIRectangle{ x_to_screenspace(x), x_to_screenspace(x + x_tick_spacing), plot_screen.b, plot_screen.b + text_height },
                         x_tick_text[tick_idx], -1, 0xffffffff, 0, nullptr);
        }
        tick_idx = 0;
        for (double y = y_tick_begin; y < ui_vp_plot->y_end; y += y_tick_spacing, ++tick_idx) {
            UIDrawLine(painter, plot_screen.l, y_to_screenspace(y), plot_screen.r, y_to_screenspace(y), ui.theme.panel1);
            UIDrawString(painter, UIRectangle{ bounds.l, bounds.l + border.l, y_to_screenspace(y) - text_height, y_to_screenspace(y) },
                         y_tick_text[tick_idx], -1, 0xffffffff, 0, nullptr);
        }

        // Drawing the Plots
        
        if (plot && !plot->empty()) {
            size_t i = plot->get_start_idx();
            
            if (plot->has_x_values) {
                int x_prev = x_to_screenspace(plot->points[i].x);
                int y_prev = y_to_screenspace(plot->points[i].y);
                ++i;
                for (size_t i = 1; i < plot->points.size(); ++i) {
                    int x = x_to_screenspace(plot->points[i].x);
                    int y = y_to_screenspace(plot->points[i].y);
                    UIDrawLine(painter, x_prev, y_prev, x, y, plot->color);
                    x_prev = x;
                    y_prev = y;
                }
            }
            else {
                int y_prev = y_to_screenspace(plot->points[i].y);
                ++i;
                for (; i < plot->points.size(); ++i) {
                    int y = y_to_screenspace(plot->points[i].y);
                    UIDrawLine(painter, x_to_screenspace(i - 1), y_prev, x_to_screenspace(i), y, plot->color);
                    y_prev = y;
                }
            }
        }

    } else if (message == UI_MSG_LEFT_DOWN) {
        UIElementFocus(element);
    }
    return 0;
}

UIViewportPlot *UIViewportPlotCreate(UIElement *parent, uint32_t flags) {
    UIViewportPlot* vp_plot = (UIViewportPlot*) UIElementCreate(sizeof(UIViewportPlot), parent, flags, _UIViewportPlotMessage, "Viewport Plot");

    vp_plot->x_begin = 0.0;
    vp_plot->y_begin = 0.0;
    vp_plot->x_end = 1.0;
    vp_plot->y_end = 1.0;
    return vp_plot;
}
