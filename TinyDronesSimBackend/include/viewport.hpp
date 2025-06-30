#include "../tinydronessim_backend.hpp"

#include <SDL2/SDL_render.h>

#include <cstddef>
#include <cstdint>
#include <math.hpp>
#include <luigi.hpp>

#include <vector>
#include <chrono>

enum Key_Event_Flags : uint8_t {
    KEY_EVENT_DOWN = 1 << 0, // else up
    KEY_EVENT_MODIFIER_CTRL = 1 << 1
};

class Environment;
class Camera;
class Frame;

struct Viewport_Plot {
    UIViewportPlot* m_ui_vp_plot;

    size_t get_start_idx();
    bool empty() { return points.empty(); }
        
    std::vector<double2> points;
    uint64_t plot_restrict_n_last_points = 0;
    bool has_x_values = false;
    uint32_t color = 0xffffffff;
};
    
struct Viewport_Frame {
    ~Viewport_Frame();

    Viewport_ID_Frame_IDX m_id_idx;
        
    Frame_ID m_frame_id;
    bool m_owns_frame = false;
        
    UIViewportFrame* m_ui_vp_frame = nullptr;
    Viewport_Frame_Flags m_flags = Viewport_Frame_Flags(0);

    struct {
        int64_t mouse_scroll_position = 0;
        int2 mouse_position = {0, 0};
        bool mouse_hovering = false;
        bool ascii_key_down[256] = {};
        bool mouse_left_down = false;
        bool mouse_right_down = false;
        bool mouse_middle_down = false;
        bool mouse_is_dragging = false;
    } m_input;

    struct {
        std::chrono::high_resolution_clock::time_point window_frame_end;
        int64_t mouse_scroll_position = 0;
        int2 mouse_position = {0, 0};
        bool mouse_is_dragging = false;
        // bool ui_hovered = false;
    } m_prev_update;

    uint32_t (*m_user_callback)(Viewport_Frame_Callback_Update) = nullptr;
    void* m_user_ptr;
    Viewport_ID m_parent_viewport;

    bool m_requires_update = true;
    void update_callback_common(Frame* frame);
        
    static UIViewportFramePixels ui_callback_get_pixels(Viewport_Frame* vp_frame, size_t width, size_t height);
    static void ui_callback_render_to_managed_window(Viewport_Frame* vp_frame, UIWindow* ui_window, size_t width, size_t height);
};

class Viewport {
public:
    static Viewport_ID create();
    ~Viewport();

    void show();

    friend Viewport_ID_Frame_IDX viewport_add_frame(Viewport_ID, Frame_ID, VP_Insert, Viewport_Frame_Flags, uint32_t (*user_callback)(Viewport_Frame_Callback_Update), void* user_ptr);
    friend Viewport_ID_Frame_IDX viewport_add_camera(Viewport_ID, Camera_ID, VP_Insert, uint32_t (*user_callback)(Viewport_Frame_Callback_Update), void* user_ptr);
    
    friend Viewport_ID_Ctrl_Panel_IDX viewport_add_ctrl_panel(Viewport_ID, VP_Insert, bool vertical);
    friend bool viewport_ctrl_add_button(Viewport_ID_Ctrl_Panel_IDX, const char* label, Viewport_Button_Callback, void* user_ptr);
    friend bool viewport_ctrl_add_slider(Viewport_ID_Ctrl_Panel_IDX, const char* label, Viewport_Slider_Callback, void* user_ptr,
                                          int steps, double min, double max, double start_pos);
    friend bool viewport_ctrl_add_label(Viewport_ID_Ctrl_Panel_IDX, const char* label);

    friend Viewport_ID_Plot_IDX viewport_add_plot(Viewport_ID, VP_Insert);
    friend bool viewport_plot_set_numbers(Viewport_ID_Plot_IDX, double* numbers, uint64_t size);
    friend bool viewport_plot_set_points(Viewport_ID_Plot_IDX, double2* points, uint64_t size);
    friend bool viewport_plot_append_number(Viewport_ID_Plot_IDX, double number);
    friend bool viewport_plot_append_numbers(Viewport_ID_Plot_IDX, double* numbers, uint64_t size);
    friend bool viewport_plot_append_point(Viewport_ID_Plot_IDX, double2 point);
    friend bool viewport_plot_append_points(Viewport_ID_Plot_IDX, double2* points, uint64_t size);
    friend bool viewport_plot_clear(Viewport_ID_Plot_IDX);
    friend bool viewport_plot_restrict_length(Viewport_ID_Plot_IDX, uint64_t length);
    
    friend bool viewport_frame_key_down(Viewport_ID_Frame_IDX, uint8_t ascii_code);
    // friend int2 viewport_frame_mouse_pos(Viewport_ID_Frame_IDX);
    // friend int64_t viewport_frame_mouse_scroll_pos(Viewport_ID_Frame_IDX);
    friend bool viewport_frame_mouse_button_down(Viewport_ID_Frame_IDX, Mouse_Button);

private:
    Viewport() {};
    Viewport_ID m_id;

    static int UIViewportMessage(UIElement *element, UIMessage message, int di, void *dp);
    static int UIViewportTabPaneMessage(UIElement* element, UIMessage message, int di, void* dp);
    static int UIViewportSplitPaneMessage(UIElement *element, UIMessage message, int di, void *dp);

    UIWindow* m_window;
    bool m_now_visible = true;
    UIElement* get_parent_ui_element(VP_Insert);
    
    std::vector<Viewport_Frame*> m_vp_frames;
    Viewport_Frame* get_vp_frame(Viewport_ID_Frame_IDX);
    
    std::vector<Viewport_Plot*> m_vp_plots;
    Viewport_Plot* get_vp_plot(Viewport_ID_Plot_IDX);

    struct Ctrl_Panel {
        UIPanel* m_ui_panel;
    };

    std::vector<Ctrl_Panel*> m_ctrl_panels;
    Ctrl_Panel* get_ctrl_panel(Viewport_ID_Ctrl_Panel_IDX);
};
