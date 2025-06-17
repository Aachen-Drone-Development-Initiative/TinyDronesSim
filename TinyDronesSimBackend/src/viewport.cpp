#include <cstdint>
#include <viewport.hpp>

#include "../tinydronessim_backend.hpp"
#include "backend/DriverEnums.h"
#include "utils/debug.h"
#include <environment.hpp>
#include <frame.hpp>
#include <camera.hpp>
#include <logging.hpp>
#include <object_manager.hpp>
#include <math.hpp>

#include <filament/Engine.h>
#include <filament/SwapChain.h>
#include <filament/Fence.h>

#include <cstring>
#include <vector>
#include <chrono>
#include <thread>
#include <iostream>

#include <SDL2/SDL.h>
#include <SDL2/SDL_events.h>
#include <SDL2/SDL_video.h>

Viewport_ID Viewport::create()
{
    Viewport* vp = new Viewport;
    
    init_luigi_if_not_init();
    
    vp->m_window = UIWindowCreate(0, 0, "TinyDronesSim Viewport", 1200, 800);
    vp->m_window->viewport = vp;
    vp->m_window->e.messageUser = UIViewportMessage;
    vp->m_id = g_objm.add_object(vp);
    // The window "animation" is just sleeping for a millisecond
    UIElementAnimate((UIElement*) vp->m_window, false);
    g_objm.viewport_activate(vp->m_id);

    return vp->m_id;
}

TDS_API Viewport_ID viewport_create() { return Viewport::create(); }

Viewport::~Viewport()
{
    UIElementDestroy(&m_window->e);
    
    for (auto vp_frame : m_vp_frames) {
        delete vp_frame;
    }
}

int Viewport::UIViewportMessage(UIElement *element, UIMessage message, int di, void *dp)
{
    UIWindow *window = (UIWindow *) element;
    Viewport* vp = window->viewport;
    
    if (message == UI_MSG_KEY_TYPED) {
        UIKeyTyped* key = (UIKeyTyped*) dp;
        if (key->code >= 0 && (size_t)key->code < 256) {
            vp->m_input_state.ascii_key_down[key->code] = true;
        }
        return 0;
    }
    else if (message == UI_MSG_KEY_RELEASED) {
        UIKeyTyped* key = (UIKeyTyped*) dp;
        if (key->code >= 0 && (size_t)key->code < 256) {
            vp->m_input_state.ascii_key_down[key->code] = false;
        }
        return 0;
    }
    else if (message == UI_MSG_MOUSE_WHEEL) { // di = delta;
        vp->m_input_state.mouse_scroll_position += di;
        return 0;
    }
    else if (message == UI_MSG_WINDOW_CLOSE) {
        UIWindowHide(window);
        return 0;
    }
    else if (message == UI_MSG_PAINT) {
        for (int i = 0; i < window->e.childCount; ++i) {
            UIElementRepaint(window->e.children[i], NULL);
        }
        return 1;
    }
    else if (message == UI_MSG_ANIMATE) {
        std::this_thread::sleep_for(std::chrono::milliseconds(2));
        return 1;
    }
    
    return 0;
}

int Viewport::UIViewportTabPaneMessage(UIElement* element, UIMessage message, int di, void* dp)
{
    UITabPane *tab_pane = (UITabPane*) element;
    
    // if (message == UI_MSG_ANIMATE) {
    //     if (tab_pane->active >= 0 && tab_pane->active < tab_pane->e.childCount) {
    //         UIElementRepaint(tab_pane->e.children[tab_pane->active], NULL);
    //     }
    //     return 1;
    // }
    if (message == UI_MSG_TAB_SELECTED) {
        UIElementMessage(element->children[tab_pane->active], UI_MSG_TAB_SELECTED, 0, 0);
    }
    if (message == UI_MSG_TAB_OTHER_SELECTED) {
        for (int i = 0; i < element->childCount; ++i) {
            UIElementMessage(element->children[i], UI_MSG_TAB_OTHER_SELECTED, 0, nullptr);
        }
    }
    return 0;
}

int Viewport::UIViewportSplitPaneMessage(UIElement* element, UIMessage message, int di, void* dp)
{
    UISplitPane *split_pane = (UISplitPane*) element;
    
    if (message == UI_MSG_TAB_SELECTED || message == UI_MSG_TAB_OTHER_SELECTED) {
        for (int i = 0; i < element->childCount; ++i) {
            UIElementMessage(element->children[i], message, 0, nullptr);
        }
    }
    return 0;
}

void Viewport::show() {
    UIWindowShow(m_window);
}

enum Location_Token {
    LOCT_RIGHT,
    LOCT_DOWN,
    LOCT_END
};

static Location_Token next_token(char** location_path)
{
    char* p = *location_path;
    if (p == nullptr || p[0] == '\0') {
        return LOCT_END;
    }
    else if (p[0] == 'r') {
        while (p[0] != '\0' && p[0] != '/') ++p;
        *location_path = p;
        return LOCT_RIGHT;
    }
    else if (p[0] == 'd') {
        while (p[0] != '\0' && p[0] != '/') ++p;
        if (p[0] == '/') ++p;
        *location_path = p;
        return LOCT_DOWN;
    }
    else {
        tds_soft_error("Failed to parse the 'location path' for adding a new "
                       "element to the Viewport. Rest-Path before failure: '%s'", *location_path);
        return LOCT_END;
    }
}

UIElement* Viewport::get_parent_ui_element(const char* location_path, const char* location_name)
{
    UIElement* element = (UIElement*) m_window;
    Location_Token loc_tkn;

    auto tab_pane_add_element = [](UITabPane* tab_pane, const char* tab_name) {
        const int max_size = 256;
        char tab_pane_str[max_size];
        if (snprintf(tab_pane_str, max_size, "%s\t%s", tab_pane->tabs, tab_name) >= max_size) {
            tds_soft_error("Tab-Panel string is longer than the max of %d", max_size);
        }
        UI_FREE(tab_pane->tabs);
        tab_pane->tabs = UIStringCopy(tab_pane_str, -1);
        return (UIElement*) tab_pane;
    };
    
    while (true)
    {
        loc_tkn = next_token((char**) &location_path);
        bool element_is_window = (element == (UIElement*) m_window);
        
        if (loc_tkn == LOCT_RIGHT || loc_tkn == LOCT_DOWN) {
            bool split_direction_matches = bool(element->flags & UI_SPLIT_PANE_VERTICAL) == (loc_tkn == LOCT_DOWN);
            
            if ((strcmp(element->cClassName, "Split Pane") == 0) && split_direction_matches) {
                tds_assert(element->childCount == 2);
                element = element->children[1];
                continue;
            }
            else {
                if (element_is_window) {
                    if (element->childCount == 0) {
                        tds_soft_error("A place for your new Viewport Element was already found, "
                                       "please remove instructions from the back of the 'location path'.");
                        return element;
                    }
                    else {
                        assert(element->childCount == 1);
                        
                        UIElement* split_pane = (UIElement*) UISplitPaneCreate(element, loc_tkn == LOCT_DOWN ? UI_SPLIT_PANE_VERTICAL : 0, 0.5);
                        split_pane->messageUser = UIViewportSplitPaneMessage;
                        UIElementChangeParent(element->children[0], split_pane, nullptr);
                    
                        if (next_token((char**) &location_path) != LOCT_END) {
                            tds_soft_error("A place for your new Viewport Element was already found, "
                                           "please remove instructions from the back of the 'location path'.");
                        }
                        return split_pane;
                    }
                }
                else {
                    UIElement* split_pane = (UIElement*) UISplitPaneCreate(element->parent, loc_tkn == LOCT_DOWN ? UI_SPLIT_PANE_VERTICAL : 0, 0.5);
                    split_pane->messageUser = UIViewportSplitPaneMessage;
                    UIElementChangeParent(element, split_pane, nullptr);
                    
                    if (next_token((char**) &location_path) != LOCT_END) {
                        tds_soft_error("A place for your new Viewport Element was already found, "
                                       "please remove instructions from the back of the 'location path'.");
                    }
                    return split_pane;
                }
            }
        }
        else if (loc_tkn == LOCT_END) {

            int tab_count = 0;
            if (location_name != nullptr) {
                const char* p = location_name;
                while (*p++) {
                    if (*p == '\t') ++tab_count;
                }
            }

            if (strcmp(element->cClassName, "Tab Pane") == 0) {
                if (tab_count != 0) {
                    tds_soft_error("For adding an element to the Viewport at the location '%s', the 'location name' "
                                   "(currently '%s'), should not contain '\\t'", location_path, location_name);
                }
                tab_pane_add_element((UITabPane*) element, location_name);
            }
            else if (element_is_window) {
                if (element->childCount != 0) {
                    tds_assert(element->childCount == 1);
                    UIElement* child = element->children[0];
                    if (strcmp(child->cClassName, "Tab Pane") == 0) {
                        if (tab_count != 0) {
                            tds_soft_error("For adding an element to the Viewport at the location '%s', the 'location name' "
                                           "(currently '%s'), should not contain '\\t'", location_path, location_name);
                        }
                        tab_pane_add_element((UITabPane*) child, location_name);
                    }
                    else {
                        if (tab_count != 1) {
                            tds_soft_error("Adding an element to the Viewport at the location '%s' requires creating a Tab-Panel, "
                                           "so you need to supply two names separated by '\\t' for the two new tabs as the "
                                           "'location_name' (currently '%s')", location_path, location_name);
                        }
                        UITabPane* tab_pane = UITabPaneCreate(element, 0, location_name);
                        tab_pane->e.messageUser = UIViewportTabPaneMessage;
                        UIElementChangeParent(element->children[0], (UIElement*) tab_pane, nullptr);
                        return (UIElement*) tab_pane;
                    }
                }
                else {
                    return element;
                }
            }
            else {
                if (tab_count != 1) {
                    tds_soft_error("Adding an element to the Viewport at the location '%s' requires creating a Tab-Panel, "
                                   "so you need to supply two names separated by '\\t' for the two new tabs as the "
                                   "'location_name' (currently '%s')", location_path, location_name);
                }
                UITabPane* tab_pane = UITabPaneCreate(element->parent, 0, location_name);
                tab_pane->e.messageUser = UIViewportTabPaneMessage;
                UIElementChangeParent(element, (UIElement*) tab_pane, nullptr);
                return (UIElement*) tab_pane;
            }
        }
        else {
            tds_unreachable();
        }
    }
    return element;
}

TDS_API bool viewport_add_frame(Viewport_ID vp_id,
                                Frame_ID frame_id,
                                Viewport_Frame_Flags flags,
                                uint32_t (*user_callback)(Viewport_Camera_Callback_Update),
                                void* user_ptr,
                                const char* location_path,
                                const char* location_name)
{
    Viewport* vp = g_objm.get_object(vp_id);
    Frame* frame = g_objm.get_object(frame_id);
    if (!vp || !frame) return false;

    uint32_2 frame_size = frame->get_size();

    Viewport::Viewport_Frame* vp_frame = new Viewport::Viewport_Frame {};
    vp_frame->m_ui_vp_frame = UIViewportFrameCreate(vp->get_parent_ui_element(location_path, location_name),
                                                    0, frame_size.x, frame_size.y);
    UIElementAnimate((UIElement*) vp_frame->m_ui_vp_frame, false);
    vp_frame->m_ui_vp_frame->callback_state = (void*) vp_frame;
    vp_frame->m_ui_vp_frame->get_new_pixels = Viewport::Viewport_Frame::ui_callback_get_pixels;
    
    vp_frame->m_frame_id = frame_id;
    vp_frame->m_flags = flags;
    vp_frame->m_user_callback = user_callback;
    vp_frame->m_user_ptr = user_ptr;
    vp_frame->m_parent_viewport = vp->m_id;

    vp->m_vp_frames.push_back(vp_frame);

    return true;
}

TDS_API bool viewport_add_camera(Viewport_ID viewport_id,
                                 Camera_ID camera_id,
                                 uint32_t (*user_callback)(Viewport_Camera_Callback_Update),
                                 void* user_ptr,
                                 const char* location_path,
                                 const char* location_name)
{
    Viewport* vp = g_objm.get_object(viewport_id);
    Camera* camera = g_objm.get_object(camera_id);
    Environment* env = g_objm.get_object(camera->get_environment());
    if (!vp || !camera || !env) return false;

    Viewport::Viewport_Frame* vp_frame = new Viewport::Viewport_Frame {};
    vp_frame->m_ui_vp_frame = UIViewportFrameCreate(vp->get_parent_ui_element(location_path, location_name),
                                                    UI_VIEWPORT_FRAME_PARENT_MANAGED_WINDOW_MODE,
                                                    20, 20);
    UIElementAnimate((UIElement*) vp_frame->m_ui_vp_frame, false);
    vp_frame->m_ui_vp_frame->callback_state = (void*) vp_frame;
    vp_frame->m_ui_vp_frame->render_to_managed_window = Viewport::Viewport_Frame::ui_callback_render_to_managed_window;
    
    fmt::SwapChain* swap_chain = env->create_swap_chain((void*) vp_frame->m_ui_vp_frame->window->window);
    vp_frame->m_frame_id = Frame::create(camera->get_environment(), camera_id, swap_chain, 20, 20);
    vp_frame->m_flags = Viewport_Frame_Flags(0);
    vp_frame->m_user_callback = user_callback;
    vp_frame->m_user_ptr = user_ptr;
    vp_frame->m_parent_viewport = vp->m_id;

    return true;
}

TDS_API bool viewport_add_controll_panel(Viewport_ID viewport_id, bool vertical, const char* location_path, const char* location_name)
{
    Viewport* vp = g_objm.get_object(viewport_id);
    if (!vp) return false;

    if (vp->m_controll.has_been_added) {
        tds_soft_error("The Controll Panel has already been added to the Viewport, and cannot be added again.");
        return false;
    }
    vp->m_controll.vertical = vertical;
    vp->m_controll.panel = UIPanelCreate(vp->get_parent_ui_element(location_path, location_name),
                                         UI_PANEL_COLOR_1 | (vertical ? 0 : UI_PANEL_HORIZONTAL));
    vp->m_controll.has_been_added = true;
    return true;
}

TDS_API bool viewport_controll_add_button(Viewport_ID viewport_id, const char* label, Viewport_Button_Callback user_callback, void* user_ptr)
{
    Viewport* vp = g_objm.get_object(viewport_id);
    if (!vp) return false;

    UIButton* button = UIButtonCreate(&vp->m_controll.panel->e, 0, label, -1);
    button->e.cp = user_ptr;
    button->invoke = user_callback;
    return true;
}

TDS_API bool viewport_controll_add_slider(Viewport_ID viewport_id, const char* label, Viewport_Slider_Callback user_callback, void* user_ptr)
{
    Viewport* vp = g_objm.get_object(viewport_id);
    if (!vp) return false;

    UISlider* slider = UISliderCreate(&vp->m_controll.panel->e, 0);
    slider->e.cp = user_ptr;
    slider->invoke = user_callback;
    return true;
}

TDS_API bool viewport_controll_add_label(Viewport_ID viewport_id, const char* label)
{
    Viewport* vp = g_objm.get_object(viewport_id);
    if (!vp) return false;

    UILabel* ui_label = UILabelCreate(&vp->m_controll.panel->e, 0, label, -1);
    return true;
}

void Viewport::Viewport_Frame::update_callback_common(Frame* frame)
{
    // Measuring frame time and sleeping if target frame time is higher.
    auto time_frame_end = std::chrono::high_resolution_clock::now();
    auto time_delta_frame = std::chrono::duration_cast<std::chrono::microseconds>(time_frame_end - m_prev_update.window_frame_end);
    m_prev_update.window_frame_end = time_frame_end;
    uint64_t time_between_frames_us = time_delta_frame.count();

    bool ui_hovered = false;
    Viewport* vp = g_objm.get_object(m_parent_viewport);
    if (vp) {
        ui_hovered = vp->m_window->hovered == (&m_ui_vp_frame->e);
    }

    int delta_mouse_wheel = 0;
    int2 delta_mouse_pos = {0, 0};
    // There shouldn't be a user input delta, when the frame was not the focused element for two consecutive updates.
    if (ui_hovered && m_prev_update.ui_hovered) {
        delta_mouse_wheel = vp->m_input_state.mouse_scroll_position - m_prev_update.mouse_scroll_position;
        delta_mouse_pos = { vp->m_window->cursorX - m_prev_update.mouse_position.x,
                            vp->m_window->cursorY - m_prev_update.mouse_position.y };
    }
    m_prev_update.mouse_position = { vp->m_window->cursorX, vp->m_window->cursorY };
    m_prev_update.mouse_scroll_position = vp->m_input_state.mouse_scroll_position;
    m_prev_update.ui_hovered = ui_hovered;

    Viewport_Camera_Callback_Update update = {
        .viewport_id = m_parent_viewport,
        .camera_id = frame->get_camera(),
        .env_id = frame->get_environment(),

        .user_ptr = m_user_ptr,
        
        .delta_time_us = time_between_frames_us,
        .delta_mouse_wheel = delta_mouse_wheel,
        .delta_mouse_pos = delta_mouse_pos,
        .ui_hovered = ui_hovered,
    };

    if (m_user_callback) {
        int callback_ret = m_user_callback(update);
    }
}

UIViewportFramePixels Viewport::Viewport_Frame::ui_callback_get_pixels(void* callback_state, size_t width, size_t height)
{
    Viewport_Frame* vp_frame = (Viewport_Frame*)callback_state;

    Frame* frame = g_objm.get_object(vp_frame->m_frame_id);
    if (!frame) return {};

    if (vp_frame->m_requires_update) {
        vp_frame->update_callback_common(frame);
    }
    
    if (vp_frame->m_flags & VIEWPORT_AUTOMATIC_RENDERING) {
        frame->set_size(width, height);
        bool rendererd = frame->render();
        vp_frame->m_requires_update = rendererd;
    }
    
    void* pixel_data;
    uint32_2 frame_size = frame->get_pixel_data(&pixel_data);

    tds_assert(frame->get_pixel_data_format() == fbackend::PixelDataFormat::RGB_INTEGER
           && frame->get_pixel_data_type() == fbackend::PixelDataType::UBYTE);

    return { .width = (size_t) frame_size.x,
             .height = (size_t) frame_size.y,
             .pixel_alignment = 3,
             .bits = (uint8_t*) pixel_data };
}

void Viewport::Viewport_Frame::ui_callback_render_to_managed_window(void* callback_state, UIWindow* ui_window, size_t width, size_t height)
{
    Viewport_Frame* vp_frame = (Viewport_Frame*)callback_state;

    Frame* frame = g_objm.get_object(vp_frame->m_frame_id);
    if (!frame) return;

    if (vp_frame->m_requires_update) {
        vp_frame->update_callback_common(frame);
    }
    
    frame->set_size(width, height);
    bool rendererd = frame->render();
    vp_frame->m_requires_update = rendererd;

    // The vulkan backend is very suseptible to changes of the window size and how that
    // affects the 'swapchain', thats why we need to ensure drawing to the swapchain is
    // completed before we might change it.
    // Environment* env = g_objm.get_object(frame->get_environment());
    // if (!env) return;
    // fmt::Fence::waitAndDestroy(env->create_fence());
}

TDS_API bool viewport_key_down(uint8_t ascii_code)
{
    Viewport* vp = g_objm.get_active_viewport();
    if (!vp) return false;

    return vp->m_input_state.ascii_key_down[ascii_code];
}

TDS_API int2 viewport_mouse_pos()
{
    Viewport* vp = g_objm.get_active_viewport();
    if (!vp) return {};

    return {vp->m_window->cursorX, vp->m_window->cursorY};
}

TDS_API int64_t viewport_mouse_scroll_pos()
{
    Viewport* vp = g_objm.get_active_viewport();
    if (!vp) return {};
    
    return vp->m_input_state.mouse_scroll_position;
}

TDS_API bool viewport_mouse_button_down(Mouse_Button button)
{
    Viewport* vp = g_objm.get_active_viewport();
    if (!vp) return {};

    switch (button) {
    case MOUSE_BUTTON_LEFT: return vp->m_window->mouse_left_down;
    case MOUSE_BUTTON_MIDDLE: return vp->m_window->mouse_middle_down;
    case MOUSE_BUTTON_RIGHT: return vp->m_window->mouse_right_down;
    }
}
