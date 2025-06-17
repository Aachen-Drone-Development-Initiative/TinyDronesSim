#include "../tinydronessim_backend.hpp"
#include <SDL2/SDL_render.h>

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

class Viewport {
public:
    static Viewport_ID create();
    ~Viewport();

    void show();

    friend bool viewport_add_frame(Viewport_ID,
                                   Frame_ID,
                                   Viewport_Frame_Flags flags,
                                   uint32_t (*user_callback)(Viewport_Camera_Callback_Update),
                                   void* user_ptr,
                                   const char* location_path,
                                   const char* location_name);
    
    friend bool viewport_add_camera(Viewport_ID,
                                    Camera_ID,
                                    uint32_t (*user_callback)(Viewport_Camera_Callback_Update),
                                    void* user_ptr,
                                    const char* location_path,
                                    const char* location_name);

    friend bool viewport_add_controll_panel(Viewport_ID, bool vertical, const char* location_path, const char* location_name);
    friend bool viewport_controll_add_button(Viewport_ID, const char* label, Viewport_Button_Callback, void* user_ptr);
    friend bool viewport_controll_add_slider(Viewport_ID, const char* label, Viewport_Slider_Callback, void* user_ptr);
    friend bool viewport_controll_add_label(Viewport_ID, const char* label);
    
    friend bool viewport_key_down(uint8_t ascii_code);
    friend int2 viewport_mouse_pos();
    friend int64_t viewport_mouse_scroll_pos();
    friend bool viewport_mouse_button_down(Mouse_Button);

private:
    Viewport() {};
    Viewport_ID m_id;

    static int UIViewportMessage(UIElement *element, UIMessage message, int di, void *dp);
    static int UIViewportTabPaneMessage(UIElement* element, UIMessage message, int di, void* dp);
    static int UIViewportSplitPaneMessage(UIElement *element, UIMessage message, int di, void *dp);

    UIWindow* m_window;
    UIElement* get_parent_ui_element(const char* panel_name, const char* location_path);
    
    struct {
        bool has_been_added = false;
        UIPanel* panel = nullptr;
        bool vertical = false;
    } m_controll;
    
    struct Viewport_Frame {
        Frame_ID m_frame_id;
        UIViewportFrame* m_ui_vp_frame = nullptr;
        Viewport_Frame_Flags m_flags = Viewport_Frame_Flags(0);

        struct {
            std::chrono::high_resolution_clock::time_point window_frame_end;
            int64_t mouse_scroll_position = 0;
            int2 mouse_position = {0, 0};
            bool ui_hovered = false;
        } m_prev_update;

        uint32_t (*m_user_callback)(Viewport_Camera_Callback_Update) = nullptr;
        void* m_user_ptr;
        Viewport_ID m_parent_viewport;

        bool m_requires_update = true;
        void update_callback_common(Frame* frame);
        
        static UIViewportFramePixels ui_callback_get_pixels(void* callback_state, size_t width, size_t height);
        static void ui_callback_render_to_managed_window(void* callback_state, UIWindow* ui_window, size_t width, size_t height);
    };
    
    std::vector<Viewport_Frame*> m_vp_frames;

    struct {
        bool ascii_key_down[256] = {};
        int64_t mouse_scroll_position = 0;
    } m_input_state;
};
