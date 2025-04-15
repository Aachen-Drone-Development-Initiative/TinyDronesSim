#include <math.hpp>

#include <SDL_scancode.h>

#define ENV_MAX_MOUSE_BUTTONS 8

enum Key_Event_Flags : uint8_t {
    KEY_EVENT_DOWN = 1 << 0, // else up
    KEY_EVENT_MODIFIER_CTRL = 1 << 1
};

struct Camera;
struct Frame;

struct SDL_Window;

struct Window {
    ~Window();
    
    SDL_Window* sdl_window = nullptr;
    Camera* camera = nullptr;
    Frame* frame = nullptr;
    double target_fps = 60;
    double last_frame_time_ms = 1;
    bool should_close = false;

    struct {
        // SDL keycodes are indices to these arrays.
        // 0 -> false, 1 -> true
        uint8_t key_event_this_frame[SDL_NUM_SCANCODES] = {};
        uint8_t key_event_prev_frame[SDL_NUM_SCANCODES] = {};
        uint8_t mouse_event_this_frame[ENV_MAX_MOUSE_BUTTONS] = {};
        uint8_t mouse_event_prev_frame[ENV_MAX_MOUSE_BUTTONS] = {};
        int mouse_x = 0;
        int mouse_y = 0;
        int mouse_x_delta = 0;
        int mouse_y_delta = 0;
        int mouse_wheel_x_delta = 0;
        int mouse_wheel_y_delta = 0;
    } input;
};
