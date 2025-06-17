
#define UI_LINUX
// #define UI_DEBUG
#define UI_IMPLEMENTATION
#include <luigi.hpp>

#include <object_manager.hpp>
#include <viewport.hpp>

#include <thread>

static bool g_called_init_luigi = false;

void init_luigi_if_not_init()
{
    if (g_called_init_luigi) return;

    UIInitialise();
    ui.theme = uiThemeDark;
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

        int w = UI_RECT_WIDTH(element->bounds);
        int h = UI_RECT_HEIGHT(element->bounds);
        /* int x = _UILinearMap(0, 0, w / vp_frame->zoom, 0, w) + element->bounds.l; */
        /* int y = _UILinearMap(0, 0, h / vp_frame->zoom, 0, h) + element->bounds.t; */

        int x = element->bounds.l;
        int y = element->bounds.t;
        
        /* UIRectangle image = UI_RECT_4(x, x + (int) (vp_frame->width * vp_frame->zoom), y, (int) (y + vp_frame->height * vp_frame->zoom)); */
        UIRectangle image = UI_RECT_4(x, x + (int) vp_frame->width, y, y + (int) vp_frame->height);
        UIRectangle bounds = UIRectangleIntersection(painter->clip, UIRectangleIntersection(vp_frame->e.bounds, image));
        if (!UI_RECT_VALID(bounds)) return 0;

        if (vp_frame->e.flags & UI_VIEWPORT_FRAME_PARENT_MANAGED_WINDOW_MODE) {
            UI_ASSERT(vp_frame->render_to_managed_window);
            UIWindowMoveResize(vp_frame->window, bounds.l, bounds.t, UI_RECT_WIDTH(element->bounds), UI_RECT_HEIGHT(element->bounds));
            // XSync(ui.display, true); // This ensures, the glx can render to a valid window, with resize completed etc.
            vp_frame->render_to_managed_window(vp_frame->callback_state, vp_frame->window, UI_RECT_WIDTH(element->bounds), UI_RECT_HEIGHT(element->bounds));
        }
        else {
            UI_ASSERT(vp_frame->get_new_pixels);
            UIViewportFramePixels pixels = vp_frame->get_new_pixels(vp_frame->callback_state, UI_RECT_WIDTH(bounds), UI_RECT_HEIGHT(bounds));

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
    
            for (size_t i = 0; i < vp_frame->height; ++i) {
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
            UIElementRepaint(element, nullptr);
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
        UIElementFocus(element);
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

UIViewportFrame *UIViewportFrameCreate(UIElement *parent, uint32_t flags, uint32_t width, uint32_t height) {
    UIViewportFrame *vp_frame = (UIViewportFrame *) UIElementCreate(sizeof(UIViewportFrame), parent, flags, _UIViewportFrameMessage, "ViewportFrame");
    vp_frame->width = width;
    vp_frame->height = height;
    vp_frame->zoom = 1.0f;
    vp_frame->visible = true;
    
    if (vp_frame->e.flags & UI_VIEWPORT_FRAME_PARENT_MANAGED_WINDOW_MODE) {
        vp_frame->window = UIWindowCreate(parent->window, UI_WINDOW_PARENT_MANAGED_SUBWINDOW, "", 20, 20);
        /* XUnmapWindow(ui.display, vp_frame->window->window); */
    }
    else {
        vp_frame->bits = (uint32_t*) UI_MALLOC(width * height * sizeof(uint32_t));
    }
    
    return vp_frame;
}
