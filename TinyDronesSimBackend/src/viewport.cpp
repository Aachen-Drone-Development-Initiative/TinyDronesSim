#include <cstddef>
#include <cstdint>
#include <viewport.hpp>

#include "../tinydronessim_backend.hpp"
#include "backend/DriverEnums.h"
#include "luigi.hpp"
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

#define CONTROLL_PANEL_SPACING 10

Viewport_ID Viewport::create()
{
    Viewport* vp = new Viewport;
    
    init_luigi_if_not_init();
    
    vp->m_window = UIWindowCreate(0, 0, "TinyDronesSim Viewport", 1200, 800);
    vp->m_window->e.messageUser = UIViewportMessage;
    vp->m_window->e.cp = (void*) vp;
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
    for (auto plot : m_vp_plots) {
        delete plot;
    }
    for (auto ctrl_panel : m_ctrl_panels) {
        delete ctrl_panel;
    }
}

int Viewport::UIViewportMessage(UIElement *element, UIMessage message, int di, void *dp)
{
    UIWindow *window = (UIWindow *) element;
    Viewport* vp = (Viewport*) element->cp;

    if (message == UI_MSG_WINDOW_CLOSE) {
        UIWindowHide(window);
        return 0;
    }
    else if (message == UI_MSG_PAINT) {
        for (uint32_t i = 0; i < window->e.childCount; ++i) {
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
        for (uint32_t i = 0; i < element->childCount; ++i) {
            UIElementMessage(element->children[i], UI_MSG_TAB_OTHER_SELECTED, 0, nullptr);
        }
    }
    return 0;
}

int Viewport::UIViewportSplitPaneMessage(UIElement* element, UIMessage message, int di, void* dp)
{
    if (message == UI_MSG_TAB_SELECTED || message == UI_MSG_TAB_OTHER_SELECTED) {
        for (uint32_t i = 0; i < element->childCount; ++i) {
            UIElementMessage(element->children[i], message, 0, nullptr);
        }
    }
    return 0;
}

void Viewport::show() {
    UIWindowShow(m_window);
}

enum Location_Token_Type {
    LOCT_RIGHT,
    LOCT_DOWN,
    LOCT_END,
    LOCT_TAB_NAME,
};

struct Location_Token {
    Location_Token_Type type = LOCT_END;
    char* tab_name_begin = nullptr;
    int tab_name_length = 0;
};

static Location_Token next_token(char** location_path)
{
    char* p = *location_path;
    if (p == nullptr || p[0] == '\0') {
        return {LOCT_END};
    }
    else if (p[0] == '/') {
        ++(*location_path);
        return {LOCT_END};
    }
    else {
        char* word_begin = p;
        int word_length = 0;
        while (p[0] != '\0' && p[0] != '/') {
            ++p;
            ++word_length;
        }

        *location_path = p;
        if (strncmp("right", word_begin, word_length) == 0) {
            return {LOCT_RIGHT};
        }
        else if (strncmp("down", word_begin, word_length) == 0) {
            return {LOCT_DOWN};
        }
        else {
            return {LOCT_TAB_NAME, word_begin, word_length};
        }
    }
}

UIElement* Viewport::get_parent_ui_element(VP_Insert vp_insert)
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

    auto tab_panel_find_tab = [](UITabPane* tab_pane, const char* loc_tab_name_begin, int loc_tab_name_length) -> UIElement* {
        const char* p = tab_pane->tabs;
        tds_assert(p);
        for (uint32_t child_idx = 0; child_idx < tab_pane->e.childCount; ++child_idx) {
            if (p[0] == '\t') ++p;
            const char* tab_name_begin = p;
            int tab_name_length = 0;
            while (p[0] != '\0' && p[0] != '\t') {
                ++p;
                ++tab_name_length;
            }
                        
            if (loc_tab_name_length == tab_name_length && strncmp(tab_name_begin, loc_tab_name_begin, tab_name_length) == 0) {
                return tab_pane->e.children[child_idx];
            }
        }
        tds_soft_error("Failed to find a Tab with the name '%.*s' when adding an element to the Viewport. "
                       "Although there is a Tab-Panel here.", loc_tab_name_length, loc_tab_name_begin);
        return nullptr;
    };
    
    while (true)
    {
        loc_tkn = next_token((char**) &vp_insert.location_path);
        bool element_is_window = (element == (UIElement*) m_window);
        
        if (loc_tkn.type == LOCT_RIGHT || loc_tkn.type == LOCT_DOWN) {
            bool split_direction_matches = bool(element->flags & UI_SPLIT_PANE_VERTICAL) == (loc_tkn.type == LOCT_DOWN);
            
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
                        
                        UIElement* split_pane = (UIElement*) UISplitPaneCreate(element, loc_tkn.type == LOCT_DOWN ? UI_SPLIT_PANE_VERTICAL : 0, 1.0 - vp_insert.weight);
                        split_pane->messageUser = UIViewportSplitPaneMessage;
                        UIElementChangeParent(element->children[0], split_pane, nullptr);
                    
                        if (next_token((char**) &vp_insert.location_path).type != LOCT_END) {
                            tds_soft_error("A place for your new Viewport Element was already found, "
                                           "please remove instructions from the back of the 'location path'.");
                        }
                        return split_pane;
                    }
                }
                else {
                    UIElement* split_pane = (UIElement*) UISplitPaneCreate(element->parent, loc_tkn.type == LOCT_DOWN ? UI_SPLIT_PANE_VERTICAL : 0, 1.0 - vp_insert.weight);
                    split_pane->messageUser = UIViewportSplitPaneMessage;
                    UIElementChangeParent(element, split_pane, nullptr);
                    
                    if (next_token((char**) &vp_insert.location_path).type != LOCT_END) {
                        tds_soft_error("A place for your new Viewport Element was already found, "
                                       "please remove instructions from the back of the 'location path'.");
                    }
                    return split_pane;
                }
            }
        }
        else if (loc_tkn.type == LOCT_TAB_NAME) {
            if (strcmp(element->cClassName, "Tab Pane") == 0) {
                UIElement* parent = tab_panel_find_tab((UITabPane*) element, loc_tkn.tab_name_begin, loc_tkn.tab_name_length);
                if (parent) {
                    element = parent;
                    continue;
                }
                else {
                    return nullptr;
                }
            }
            else if (element_is_window) {
                if (element->childCount != 0) {
                    tds_assert(element->childCount == 1);
                    UIElement* child = element->children[0];
                    if (strcmp(child->cClassName, "Tab Pane") == 0) {
                        UIElement* parent = tab_panel_find_tab((UITabPane*) child, loc_tkn.tab_name_begin, loc_tkn.tab_name_length);
                        if (parent) {
                            element = parent;
                            continue;
                        }
                        else {
                            return nullptr;
                        }
                    }
                }
            }
            
            tds_soft_error("Didn't know what to do with the word '%.*s' when adding an element to the Viewport. "
                           "There is no Tab-Panel here, to look for a Tab with this name.", loc_tkn.tab_name_length, loc_tkn.tab_name_begin);
            return nullptr;
        }
        else if (loc_tkn.type == LOCT_END) {

            // Duplicating the location_name because that is probably a non-writable string-literal.
            char* fixed_location_name = (char*) malloc(strlen(vp_insert.location_name));
            strcpy(fixed_location_name, vp_insert.location_name);
            
            int tab_count = 0;
            if (vp_insert.location_name) {
                char* p = fixed_location_name;
                while (*p++) {
                    if (*p == '|') {
                        ++tab_count;
                        *p = '\t'; // rewriting '|' to '\t' because this is how luigi distinguishes tabs
                    }
                }
            }

            if (strcmp(element->cClassName, "Tab Pane") == 0) {
                if (tab_count != 0) {
                    tds_soft_error("For adding an element to the Viewport at the location '%s', the 'location name' "
                                   "(currently '%s'), should not contain '\\t'", vp_insert.location_path, fixed_location_name);
                }
                tab_pane_add_element((UITabPane*) element, fixed_location_name);
            }
            else if (element_is_window) {
                if (element->childCount != 0) {
                    tds_assert(element->childCount == 1);
                    UIElement* child = element->children[0];
                    if (strcmp(child->cClassName, "Tab Pane") == 0) {
                        if (tab_count != 0) {
                            tds_soft_error("For adding an element to the Viewport at the location '%s', the 'location name' "
                                           "(currently '%s'), should not contain '\\t'", vp_insert.location_path, fixed_location_name);
                        }
                        tab_pane_add_element((UITabPane*) child, fixed_location_name);
                        return child;
                    }
                    else {
                        if (tab_count != 1) {
                            tds_soft_error("Adding an element to the Viewport at the location '%s' requires creating a Tab-Panel, "
                                           "so you need to supply two names separated by '\\t' for the two new tabs as the "
                                           "'location_name' (currently '%s')", vp_insert.location_path, fixed_location_name);
                        }
                        UITabPane* tab_pane = UITabPaneCreate(element, 0, fixed_location_name);
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
                                   "'location_name' (currently '%s')", vp_insert.location_path, fixed_location_name);
                }
                UITabPane* tab_pane = UITabPaneCreate(element->parent, 0, fixed_location_name);
                tab_pane->e.messageUser = UIViewportTabPaneMessage;
                UIElementChangeParent(element, (UIElement*) tab_pane, nullptr);
                return (UIElement*) tab_pane;
            }

            free(fixed_location_name);
        }
        else {
            tds_unreachable();
        }
    }
    return element;
}

/////////////////////////////////
// Viewport Frame
/////////////////////////////////

Viewport_Frame::~Viewport_Frame()
{
    if (m_owns_frame) {
        g_objm.destroy_object(m_frame_id);
    }
}

TDS_API Viewport_ID_Frame_IDX viewport_add_frame(Viewport_ID vp_id,
                                                 Frame_ID frame_id,
                                                 VP_Insert vp_insert,
                                                 Viewport_Frame_Flags flags,
                                                 uint32_t (*user_callback)(Viewport_Frame_Callback_Update),
                                                 void* user_ptr)
{
    Viewport* vp = g_objm.get_object(vp_id);
    Frame* frame = g_objm.get_object(frame_id);
    if (!vp || !frame) return {{TDS_INVALID_ID}, TDS_INVALID_IDX};

    uint32_2 frame_size = frame->get_size();

    Viewport_Frame* vp_frame = new Viewport_Frame {};

    UIElement* ui_parent = vp->get_parent_ui_element(vp_insert);
    if (!ui_parent) return {{TDS_INVALID_ID}, TDS_INVALID_IDX};
    
    vp_frame->m_ui_vp_frame = UIViewportFrameCreate(ui_parent, 0, frame_size.x, frame_size.y, vp_frame);
    UIElementAnimate((UIElement*) vp_frame->m_ui_vp_frame, false);
    vp_frame->m_ui_vp_frame->get_new_pixels = Viewport_Frame::ui_callback_get_pixels;
    
    vp_frame->m_frame_id = frame_id;
    vp_frame->m_flags = flags;
    vp_frame->m_user_callback = user_callback;
    vp_frame->m_user_ptr = user_ptr;
    vp_frame->m_parent_viewport = vp->m_id;

    vp->m_vp_frames.push_back(vp_frame);
    vp_frame->m_id_idx = { vp_id, (int) vp->m_vp_frames.size() - 1};
    return vp_frame->m_id_idx;
}

TDS_API Viewport_ID_Frame_IDX viewport_add_camera(Viewport_ID vp_id,
                                                  Camera_ID camera_id,
                                                  VP_Insert vp_insert,
                                                  uint32_t (*user_callback)(Viewport_Frame_Callback_Update),
                                                  void* user_ptr)
{
    Viewport* vp = g_objm.get_object(vp_id);
    Camera* camera = g_objm.get_object(camera_id);
    Environment* env = g_objm.get_object(camera->get_environment());
    if (!vp || !camera || !env) return {{TDS_INVALID_ID}, TDS_INVALID_IDX};

    Viewport_Frame* vp_frame = new Viewport_Frame {};
    
    UIElement* ui_parent = vp->get_parent_ui_element(vp_insert);
    if (!ui_parent) return {{TDS_INVALID_ID}, TDS_INVALID_IDX};
    
    vp_frame->m_ui_vp_frame = UIViewportFrameCreate(ui_parent, UI_VIEWPORT_FRAME_PARENT_MANAGED_WINDOW_MODE, 20, 20, vp_frame);
    UIElementAnimate((UIElement*) vp_frame->m_ui_vp_frame, false);
    vp_frame->m_ui_vp_frame->render_to_managed_window = Viewport_Frame::ui_callback_render_to_managed_window;
    
    fmt::SwapChain* swap_chain = env->create_swap_chain((void*) vp_frame->m_ui_vp_frame->window->window);
    vp_frame->m_frame_id = Frame::create(camera->get_environment(), camera_id, swap_chain, 20, 20);
    vp_frame->m_owns_frame = true;
    vp_frame->m_flags = Viewport_Frame_Flags(0);
    vp_frame->m_user_callback = user_callback;
    vp_frame->m_user_ptr = user_ptr;
    vp_frame->m_parent_viewport = vp->m_id;

    vp->m_vp_frames.push_back(vp_frame);
    vp_frame->m_id_idx = { vp_id, (int) vp->m_vp_frames.size() - 1};
    return vp_frame->m_id_idx;
}

Viewport_Frame* Viewport::get_vp_frame(Viewport_ID_Frame_IDX id_idx)
{
    if (id_idx.idx >= 0 && (size_t) id_idx.idx < m_vp_frames.size()) {
        return m_vp_frames[id_idx.idx];
    }
    else {
        tds_soft_error("The Viewport Frame with index '%d' does not exist for the Viewport '%d'", id_idx.idx, id_idx.id);
        return nullptr;
    }
}

void Viewport_Frame::update_callback_common(Frame* frame)
{
    // Measuring frame time and sleeping if target frame time is higher.
    auto time_frame_end = std::chrono::high_resolution_clock::now();
    auto time_delta_frame = std::chrono::duration_cast<std::chrono::microseconds>(time_frame_end - m_prev_update.window_frame_end);
    m_prev_update.window_frame_end = time_frame_end;
    uint64_t time_between_frames_us = time_delta_frame.count();

    int2 delta_mouse_drag = {0, 0};
    // Drag delta can only exist between to 'drag'-frames 
    if (m_input.mouse_is_dragging && m_prev_update.mouse_is_dragging) {
        delta_mouse_drag = {m_input.mouse_position.x - m_prev_update.mouse_position.x,
                            m_input.mouse_position.y - m_prev_update.mouse_position.y};
    }

    Viewport_Frame_Callback_Update update = {
        .vp_frame_id_idx = m_id_idx,
        .camera_id = frame->get_camera(),
        .env_id = frame->get_environment(),

        .user_ptr = m_user_ptr,
        
        .delta_time_us = time_between_frames_us,
        .delta_mouse_wheel = (int) (m_input.mouse_scroll_position - m_prev_update.mouse_scroll_position),
        .delta_mouse_drag = delta_mouse_drag,
        .ui_hovered = (m_ui_vp_frame->e.window->hovered == (&m_ui_vp_frame->e)),
    };

    m_prev_update.mouse_position = m_input.mouse_position;
    m_prev_update.mouse_scroll_position = m_input.mouse_scroll_position;
    m_prev_update.mouse_is_dragging = m_input.mouse_is_dragging;

    if (m_user_callback) {
        [[maybe_unused]] int callback_ret = m_user_callback(update);
    }
}

UIViewportFramePixels Viewport_Frame::ui_callback_get_pixels(Viewport_Frame* vp_frame, size_t width, size_t height)
{
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

void Viewport_Frame::ui_callback_render_to_managed_window(Viewport_Frame* vp_frame, UIWindow* ui_window, size_t width, size_t height)
{
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

/////////////////////////////////
// Controll Panel
/////////////////////////////////

TDS_API Viewport_ID_Ctrl_Panel_IDX viewport_add_ctrl_panel(Viewport_ID vp_id, VP_Insert vp_insert, bool vertical)
{
    Viewport* vp = g_objm.get_object(vp_id);
    if (!vp) return {{TDS_INVALID_ID}, TDS_INVALID_IDX};

    UIElement* ui_parent = vp->get_parent_ui_element(vp_insert);
    if (!ui_parent) return {{TDS_INVALID_ID}, TDS_INVALID_IDX};

    Viewport::Ctrl_Panel* ctrl_panel = new Viewport::Ctrl_Panel;
    
    ctrl_panel->m_ui_panel = UIPanelCreate(ui_parent, UI_PANEL_COLOR_1 | (vertical ? 0 : UI_PANEL_HORIZONTAL));
    vp->m_ctrl_panels.push_back(ctrl_panel);
    
    return {vp_id, (int) vp->m_ctrl_panels.size() - 1};
}

Viewport::Ctrl_Panel* Viewport::get_ctrl_panel(Viewport_ID_Ctrl_Panel_IDX panel_id_idx)
{
    if (panel_id_idx.idx >= 0 && (size_t) panel_id_idx.idx < m_ctrl_panels.size()) {
        return m_ctrl_panels[panel_id_idx.idx];
    }
    else {
        tds_soft_error("The Controll Panel with index '%d' does not exist for the Viewport '%d'", panel_id_idx.idx, panel_id_idx.id);
        return nullptr;
    }    
}

TDS_API bool viewport_ctrl_add_button(Viewport_ID_Ctrl_Panel_IDX panel_id_idx, const char* label, Viewport_Button_Callback user_callback, void* user_ptr)
{
    Viewport* vp = g_objm.get_object(panel_id_idx.id);
    if (!vp) return false;

    Viewport::Ctrl_Panel* ctrl_panel = vp->get_ctrl_panel(panel_id_idx);
    if (!ctrl_panel) return false;

    UISpacerCreate(&ctrl_panel->m_ui_panel->e, 0, CONTROLL_PANEL_SPACING, CONTROLL_PANEL_SPACING);
    UIButton* button = UIButtonCreate(&ctrl_panel->m_ui_panel->e, 0, label, -1);
    button->e.cp = user_ptr;
    button->invoke = user_callback;
    return true;
}

TDS_API bool viewport_ctrl_add_slider(Viewport_ID_Ctrl_Panel_IDX panel_id_idx, const char* label, Viewport_Slider_Callback user_callback, void* user_ptr,
                                      int steps, double min, double max, double start_pos)
{
    Viewport* vp = g_objm.get_object(panel_id_idx.id);
    if (!vp) return false;

    Viewport::Ctrl_Panel* ctrl_panel = vp->get_ctrl_panel(panel_id_idx);
    if (!ctrl_panel) return false;

    UISpacerCreate(&ctrl_panel->m_ui_panel->e, 0, CONTROLL_PANEL_SPACING, CONTROLL_PANEL_SPACING);
    UIPanel* slider_panel = UIPanelCreate(&ctrl_panel->m_ui_panel->e, UI_PANEL_HORIZONTAL | UI_PANEL_COLOR_1);
    UILabelCreate(&slider_panel->e, 0, label, -1);
    UISpacerCreate(&slider_panel->e, 0, 5, 5);
    UISlider* slider = UISliderCreate(&slider_panel->e, 0);
    slider->position = linear_map(start_pos, min, max, 0.0, 1.0);
    slider->steps = steps;
    slider->min = min;
    slider->max = max;
    slider->e.cp = user_ptr;
    slider->invoke = user_callback;
    return true;
}

TDS_API bool viewport_ctrl_add_label(Viewport_ID_Ctrl_Panel_IDX panel_id_idx, const char* label)
{
    Viewport* vp = g_objm.get_object(panel_id_idx.id);
    if (!vp) return false;

    Viewport::Ctrl_Panel* ctrl_panel = vp->get_ctrl_panel(panel_id_idx);
    if (!ctrl_panel) return false;

    UILabelCreate(&ctrl_panel->m_ui_panel->e, 0, label, -1);
    UISpacerCreate(&ctrl_panel->m_ui_panel->e, 0, CONTROLL_PANEL_SPACING, CONTROLL_PANEL_SPACING);
    return true;
}

/////////////////////////////////
// Viewport Plot
/////////////////////////////////

TDS_API Viewport_ID_Plot_IDX viewport_add_plot(Viewport_ID vp_id, VP_Insert vp_insert)
{
    Viewport* vp = g_objm.get_object(vp_id);
    if (!vp) return {{TDS_INVALID_ID}, TDS_INVALID_IDX};

    UIElement* ui_parent = vp->get_parent_ui_element(vp_insert);
    if (!ui_parent) return {{TDS_INVALID_ID}, TDS_INVALID_IDX};

    Viewport_Plot* vp_plot = new Viewport_Plot;
    vp_plot->m_ui_vp_plot = UIViewportPlotCreate(ui_parent, 0);
    vp_plot->m_ui_vp_plot->vp_plot = vp_plot;
    vp->m_vp_plots.push_back(vp_plot);
    UIElementRepaint(&vp_plot->m_ui_vp_plot->e, nullptr);
    
    return {vp_id, (int) vp->m_vp_plots.size() - 1};
}

Viewport_Plot* Viewport::get_vp_plot(Viewport_ID_Plot_IDX plot_id_idx)
{
    if (plot_id_idx.idx >= 0 && (size_t) plot_id_idx.idx < m_vp_plots.size()) {
        return m_vp_plots[plot_id_idx.idx];
    }
    else {
        tds_soft_error("The Viewport Plot with index '%d' does not exist for the Viewport '%d'", plot_id_idx.idx, plot_id_idx.id);
        return nullptr;
    }
}

size_t Viewport_Plot::get_start_idx()
{
    if (plot_restrict_n_last_points != 0 && points.size() > plot_restrict_n_last_points) {
        return points.size() - plot_restrict_n_last_points;
    }
    else {
        return 0;
    }
}

TDS_API bool viewport_plot_set_numbers(Viewport_ID_Plot_IDX plot_id_idx, double* numbers, uint64_t size)
{
    Viewport* vp = g_objm.get_object(plot_id_idx.id);
    if (!vp) return false;

    Viewport_Plot* plot = vp->get_vp_plot(plot_id_idx);
    if (!plot) return false;
    
    plot->points.clear();
    plot->points.resize(size);
    for (uint64_t i = 0; i < size; ++i) {
        plot->points[i].y = numbers[i];
    }
    UIElementRepaint(&plot->m_ui_vp_plot->e, nullptr);

    return true;
}

TDS_API bool viewport_plot_set_points(Viewport_ID_Plot_IDX plot_id_idx, double2* points, uint64_t size)
{
    Viewport* vp = g_objm.get_object(plot_id_idx.id);
    if (!vp) return false;

    Viewport_Plot* plot = vp->get_vp_plot(plot_id_idx);
    if (!plot) return false;
    
    plot->points.clear();
    plot->points.resize(size);
    for (uint64_t i = 0; i < size; ++i) {
        plot->points[i] = points[i];
    }
    UIElementRepaint(&plot->m_ui_vp_plot->e, nullptr);

    return true;
}

TDS_API bool viewport_plot_append_number(Viewport_ID_Plot_IDX plot_id_idx, double number)
{
    Viewport* vp = g_objm.get_object(plot_id_idx.id);
    if (!vp) return false;
    
    Viewport_Plot* plot = vp->get_vp_plot(plot_id_idx);
    if (!plot) return false;
    
    if (plot->points.empty()) {
        plot->has_x_values = false;
    }
    else if (plot->has_x_values) {
        tds_soft_error("The plot contains 2D Points, but you tried to add 1D Values. Clear the plot first!");
    }

    plot->points.push_back({0, number});
    UIElementRepaint(&plot->m_ui_vp_plot->e, nullptr);
    
    return true;
}

TDS_API bool viewport_plot_append_numbers(Viewport_ID_Plot_IDX plot_id_idx, double* numbers, uint64_t size)
{
    Viewport* vp = g_objm.get_object(plot_id_idx.id);
    if (!vp) return false;
    
    Viewport_Plot* plot = vp->get_vp_plot(plot_id_idx);
    if (!plot) return false;
    
    if (plot->points.empty()) {
        plot->has_x_values = false;
    }
    else if (plot->has_x_values) {
        tds_soft_error("The plot contains 2D Points, but you tried to add 1D Values. Clear the plot first!");
    }

    for (uint64_t i = 0; i < size; ++i) {
        plot->points.push_back({0, numbers[i]});
    }
    UIElementRepaint(&plot->m_ui_vp_plot->e, nullptr);
    return true;
}

TDS_API bool viewport_plot_append_point(Viewport_ID_Plot_IDX plot_id_idx, double2 point)
{
    Viewport* vp = g_objm.get_object(plot_id_idx.id);
    if (!vp) return false;
    
    Viewport_Plot* plot = vp->get_vp_plot(plot_id_idx);
    if (!plot) return false;

    if (plot->points.empty()) {
        plot->has_x_values = true;
    }
    else if (!plot->has_x_values) {
        tds_soft_error("The plot contains 1D Values, but you tried to add a 2D Point. Clear the plot first!");
    }

    plot->points.push_back(point);
    UIElementRepaint(&plot->m_ui_vp_plot->e, nullptr);
    return true;
}

TDS_API bool viewport_plot_append_points(Viewport_ID_Plot_IDX plot_id_idx, double2* points, uint64_t size)
{
    Viewport* vp = g_objm.get_object(plot_id_idx.id);
    if (!vp) return false;
    
    Viewport_Plot* plot = vp->get_vp_plot(plot_id_idx);
    if (!plot) return false;

    if (plot->points.empty()) {
        plot->has_x_values = true;
    }
    else if (!plot->has_x_values) {
        tds_soft_error("The plot contains 1D Values, but you tried to add 2D Points. Clear the plot first!");
    }

    for (uint64_t i = 0; i < size; ++i) {
        plot->points.push_back(points[i]);
    }
    UIElementRepaint(&plot->m_ui_vp_plot->e, nullptr);
    return true;
}

TDS_API bool viewport_plot_clear(Viewport_ID_Plot_IDX plot_id_idx)
{
    Viewport* vp = g_objm.get_object(plot_id_idx.id);
    if (!vp) return false;
    
    Viewport_Plot* plot = vp->get_vp_plot(plot_id_idx);
    if (!plot) return false;
    
    plot->points.clear();
    UIElementRepaint(&plot->m_ui_vp_plot->e, nullptr);
    return true;
}

TDS_API bool viewport_plot_restrict_length(Viewport_ID_Plot_IDX plot_id_idx, uint64_t length)
{
    Viewport* vp = g_objm.get_object(plot_id_idx.id);
    if (!vp) return false;
    
    Viewport_Plot* plot = vp->get_vp_plot(plot_id_idx);
    if (!plot) return false;

    plot->plot_restrict_n_last_points = length;
    UIElementRepaint(&plot->m_ui_vp_plot->e, nullptr);
    return true;
}

/////////////////////////////////
// User input
/////////////////////////////////

TDS_API bool viewport_frame_key_down(Viewport_ID_Frame_IDX id_idx, uint8_t ascii_code)
{
    Viewport* vp = g_objm.get_object(id_idx.id);
    if (!vp) return false;

    Viewport_Frame* vp_frame = vp->get_vp_frame(id_idx);
    if (!vp_frame) return false;
    
    return vp_frame->m_input.ascii_key_down[ascii_code];
}

// TDS_API int2 viewport_frame_mouse_pos(Viewport_ID_Frame_IDX id_idx)
// {
//     Viewport* vp = g_objm.get_object(id_idx.id);
//     if (!vp) return {};

//     Viewport_Frame* vp_frame = vp->get_vp_frame(id_idx);
//     if (!vp_frame) return {};

//     return vp_frame->m_input.mouse_position;
// }

// TDS_API int64_t viewport_frame_mouse_scroll_pos(Viewport_ID_Frame_IDX id_idx)
// {
//     Viewport* vp = g_objm.get_object(id_idx.id);
//     if (!vp) return {};

//     Viewport_Frame* vp_frame = vp->get_vp_frame(id_idx);
//     if (!vp_frame) return {};
    
//     return vp_frame->m_input.mouse_scroll_position;
// }

TDS_API bool viewport_frame_mouse_button_down(Viewport_ID_Frame_IDX id_idx, Mouse_Button button)
{
    Viewport* vp = g_objm.get_object(id_idx.id);
    if (!vp) return {};

    Viewport_Frame* vp_frame = vp->get_vp_frame(id_idx);
    if (!vp_frame) return {};

    switch (button) {
    case MOUSE_BUTTON_LEFT: return vp_frame->m_input.mouse_left_down;
    case MOUSE_BUTTON_MIDDLE: return vp_frame->m_input.mouse_middle_down;
    case MOUSE_BUTTON_RIGHT: return vp_frame->m_input.mouse_right_down;
    }
}
