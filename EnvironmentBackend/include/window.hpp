#include "../environments.hpp"
#include "SDL_render.h"

#include <cstdint>
#include <math.hpp>

#include <SDL_scancode.h>
#include <SDL_joystick.h>

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
    uint32_t sdl_window_id = 0;
    SDL_Joystick* joystick = nullptr;
    Camera* camera = nullptr;
    Frame* frame = nullptr;
    double target_fps = 60;
    double last_frame_time_ms = 1;
    bool has_mouse_focus = false;
    bool is_visible = true;

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

        int16_t joystick_axes_raw[ENV_MAX_JOYSTICK_AXES] = {}; // index is axis idx
        // We add 1 to ENV_MAX_JOYSTICK_AXES, because this is the simplest way to make julias 1 based indexing compatible.
        // Effectively we ignore the element at 0, 0 is also used as the invalid axis type.
        int16_t joystick_axes_types_raw_min[ENV_MAX_JOYSTICK_AXES + 1] = {}; // index is axis type
        int16_t joystick_axes_types_raw_max[ENV_MAX_JOYSTICK_AXES + 1] = {}; // index is axis type
        int16_t joystick_axes_types_raw_zero[ENV_MAX_JOYSTICK_AXES + 1] = {}; // index is axis type
        uint8_t joystick_axis_type_to_idx[ENV_MAX_JOYSTICK_AXES + 1] = {}; // index is axis type
    } input;
};
