#include "../environments.h"
#include <SDL2/SDL_error.h>
#include <cstddef>
#include <cstdint>
#include <window.hpp>

#include <environment.hpp>
#include <frame.hpp>
#include <camera.hpp>
#include <logging.hpp>
#include <object_manager.hpp>

#include <filament/Engine.h>
#include <filament/SwapChain.h>

#include <cstring>

#include <SDL.h>
#include <SDL_events.h>
#include <SDL_video.h>

#define SDL_VIDEO_DRIVER_X11
#define SDL_VIDEO_DRIVER_WAYLAND
// We need this namespace because 'SDL_syswm.h' includes
// 'X.h' which also defines the symbol 'Window'.
namespace syswm {
#include <SDL_syswm.h>
}

static void* get_native_window(SDL_Window* sdlWindow) {
    syswm::SDL_SysWMinfo wmi;
    SDL_VERSION(&wmi.version);
    if(!SDL_GetWindowWMInfo(sdlWindow, &wmi)) {
        env_hard_error("SDL version not supported.");
    }
    if (wmi.subsystem == syswm::SDL_SYSWM_X11) {
        syswm::Window win = (syswm::Window) wmi.info.x11.window;
        return (void *) win;
    }
    else if (wmi.subsystem == syswm::SDL_SYSWM_WAYLAND) {
        int width = 0;
        int height = 0;
        SDL_GetWindowSize(sdlWindow, &width, &height);

        // Static is used here to allocate the struct pointer for the lifetime of the program.
        // Without static the valid struct quickyly goes out of scope, and ends with seemingly
        // random segfaults.
        static struct {
            struct syswm::wl_display *display;
            struct syswm::wl_surface *surface;
            uint32_t width;
            uint32_t height;
        } wayland {
            wmi.info.wl.display,
            wmi.info.wl.surface,
            static_cast<uint32_t>(width),
            static_cast<uint32_t>(height)
        };
        return (void *) &wayland;
    }
    return nullptr;
}

UUID create_window(UUID camera_id, int target_fps, const char* name)
{
    Camera* camera = g_objm.get_as_camera(camera_id);
    if (!camera) return ENV_INVALID_UUID;

    Window* window = new Window;
    window->camera = camera;
    window->target_fps = target_fps;

    if (SDL_InitSubSystem(SDL_INIT_EVENTS | SDL_INIT_JOYSTICK) < 0) { 
        env_hard_error("Failed to initialize SDL: %s", SDL_GetError());
    }
    SDL_JoystickEventState(SDL_ENABLE);

    uint32_t sdl_window_flags = SDL_WINDOW_SHOWN
        | SDL_WINDOW_ALLOW_HIGHDPI
        | SDL_WINDOW_RESIZABLE;
    
    window->sdl_window = SDL_CreateWindow(name, 0, 0,
                                          get_camera_image_width(camera),
                                          get_camera_image_height(camera),
                                          sdl_window_flags);
    window->sdl_window_id = SDL_GetWindowID(window->sdl_window);
    
    window->frame = __create_frame(window->camera->env,
                                   window->camera->env->engine->createSwapChain(
                                       get_native_window(window->sdl_window)));

    Env_Object obj = g_objm.add_object(window);
    g_objm.set_active_window(window, obj.id);
    
    return obj.id;
}

Window::~Window()
{
    delete frame;
    SDL_DestroyWindow(sdl_window);
}

static bool window_process_event(UUID window_id, SDL_Event event, bool& destroy_this_window)
{
    destroy_this_window = false;
    Window* window = g_objm.get_as_window(window_id);
    if (!window) return false;

    switch (event.type) {
    case SDL_WINDOWEVENT:
        if (event.window.windowID == window->sdl_window_id)
        {
            switch (event.window.event) {
            case SDL_WINDOWEVENT_RESIZED:
            {
                int width, height;
                SDL_GL_GetDrawableSize(window->sdl_window, &width, &height);
                set_camera_image_size(window->camera, width, height);
                break;
            }
                
            case SDL_WINDOWEVENT_CLOSE:
                SDL_HideWindow(window->sdl_window);
                destroy_this_window = true;
                break;

            case SDL_WINDOWEVENT_ENTER:
                window->has_mouse_focus = true;
                break;

            case SDL_WINDOWEVENT_LEAVE:
                window->has_mouse_focus = false;
                break;

            case SDL_WINDOWEVENT_SHOWN:
                window->is_visible = true;
                break;

            case SDL_WINDOWEVENT_HIDDEN:
                window->is_visible = false;
                break;

            default:
                break;
            }
        }
        break;
            
    case SDL_MOUSEMOTION:
        if (event.motion.windowID == window->sdl_window_id) {
            window->input.mouse_x = event.motion.x;
            window->input.mouse_y = event.motion.y;
            window->input.mouse_x_delta = event.motion.xrel;
            window->input.mouse_y_delta = event.motion.yrel;
        }
        break;
            
    case SDL_MOUSEBUTTONDOWN:
        if (event.button.windowID == window->sdl_window_id) {
            window->input.mouse_event_this_frame[event.button.button] = 1u;
        }
        break;

    case SDL_MOUSEBUTTONUP:
        if (event.button.windowID == window->sdl_window_id) {
            window->input.mouse_event_this_frame[event.button.button] = 0u;
        }
        break;
            
    case SDL_MOUSEWHEEL:
        if (event.wheel.windowID == window->sdl_window_id) {
            window->input.mouse_wheel_x_delta = event.wheel.x;
            window->input.mouse_wheel_y_delta = event.wheel.y;
        }
        break;
            
    case SDL_KEYDOWN:
        if (event.key.windowID == window->sdl_window_id) {
            window->input.key_event_this_frame[event.key.keysym.scancode] = KEY_EVENT_DOWN;
        }
        break;
        
    case SDL_KEYUP:
        if (event.key.windowID == window->sdl_window_id) {
            window->input.key_event_this_frame[event.key.keysym.scancode] = 0u;
        }
        break;

    case SDL_JOYAXISMOTION:
        window->input.joystick_axes_raw[event.jaxis.axis] = event.jaxis.value;
        break;
        
    default:
        break;
    }
    return true;
}

bool update_window()
{
    UUID window_id = g_objm.get_active_window_id();
    Window* window = g_objm.get_active_window();
    if (!window) return false;
    
    /*
     * Rendering
     *
     * Because the SDL-event response is handled outside (by the user) we
     * render first and collect events afterwards to reduce latency.
     */
    
    render_frame(window->camera, window->frame);

    /*
     * Event Handling
     */

    // resetting input state
    std::memcpy(window->input.key_event_prev_frame, window->input.key_event_this_frame, SDL_NUM_SCANCODES);
    std::memcpy(window->input.mouse_event_prev_frame, window->input.mouse_event_this_frame, ENV_MAX_MOUSE_BUTTONS);
    window->input.mouse_x_delta = 0;
    window->input.mouse_y_delta = 0;
    window->input.mouse_wheel_x_delta = 0;
    window->input.mouse_wheel_y_delta = 0;

    // Consuming SDL events

    SDL_Event event;

    const std::vector<UUID>& all_window_ids = g_objm.get_all_window_ids_vector();
    std::vector<UUID> windows_to_destroy;

    while (SDL_PollEvent(&event) != 0)
    {
        // Every window gets the chance to process the event.
        for (size_t i = 0; i < all_window_ids.size(); ++i) {
            bool destroy_this_window = false;
            window_process_event(all_window_ids[i], event, destroy_this_window);
            if (destroy_this_window) {
                windows_to_destroy.push_back(all_window_ids[i]);
            }
        }
    }

    /*
     * FPS correction
     * sleep so that 'target_fps' is never surpassed
     */
    
    double prev_image_time_ms = update_image_time(window->camera);
    double time_delta_ms = window->camera->image_time_ms - prev_image_time_ms;
    double target_time_delta_ms = 1.0e3 / window->target_fps;
    window->last_frame_time_ms = time_delta_ms;
    if (time_delta_ms < target_time_delta_ms) {
        SDL_Delay(target_time_delta_ms - time_delta_ms);
        window->last_frame_time_ms = target_time_delta_ms;
    }

    for (UUID destroy_window_id : windows_to_destroy) {
        g_objm.destroy_object(destroy_window_id);
    }

    // Check if now all windows are deleted and, if that is the case, quit SDL.
    if (g_objm.get_all_window_ids_vector().empty()) {
        SDL_Quit();
        return true;
    }
    
    return true;
}

double get_last_frame_time_of_window_ms()
{
    Window* window = g_objm.get_active_window();
    if (!window) return 0.0;
    
    return window->last_frame_time_ms;
}

bool focus_input_to_window()
{
    Window* window = g_objm.get_active_window();
    if (!window) return false;
    
    return SDL_SetWindowInputFocus(window->sdl_window) == 0;
}

bool is_key_down(SDL_Scancode key)
{
    Window* window = g_objm.get_active_window();
    if (!window) return false;
    
    return window->input.key_event_this_frame[key] & KEY_EVENT_DOWN;
}

bool is_key_pressed(SDL_Scancode key)
{
    Window* window = g_objm.get_active_window();
    if (!window) return false;
    
    return (window->input.key_event_this_frame[key] & KEY_EVENT_DOWN)
        && !(window->input.key_event_prev_frame[key] & KEY_EVENT_DOWN);
}

bool is_key_up(SDL_Scancode key)
{
    Window* window = g_objm.get_active_window();
    if (!window) return false;

    return !(window->input.key_event_this_frame[key] & KEY_EVENT_DOWN);
}

bool is_key_released(SDL_Scancode key)
{
    Window* window = g_objm.get_active_window();
    if (!window) return false;

    return !(window->input.key_event_this_frame[key] & KEY_EVENT_DOWN)
        && (window->input.key_event_prev_frame[key] & KEY_EVENT_DOWN);
}

bool is_mouse_button_down(Mouse_Button button)
{
    Window* window = g_objm.get_active_window();
    if (!window) return false;

    return window->input.mouse_event_this_frame[button];
}

bool is_mouse_button_pressed(Mouse_Button button)
{
    Window* window = g_objm.get_active_window();
    if (!window) return false;

    return window->input.mouse_event_this_frame[button]
        && !(window->input.mouse_event_prev_frame[button]);
}

bool is_mouse_button_up(Mouse_Button button)
{
    Window* window = g_objm.get_active_window();
    if (!window) return false;

    return !window->input.mouse_event_this_frame[button];
}

bool is_mouse_button_released(Mouse_Button button)
{
    Window* window = g_objm.get_active_window();
    if (!window) return false;

    return !(window->input.mouse_event_this_frame[button])
        && window->input.mouse_event_prev_frame[button];    
}
    
int2 get_mouse_pos()
{
    Window* window = g_objm.get_active_window();
    if (!window) return int2{};

    return {window->input.mouse_x, window->input.mouse_y};
}

int2 get_mouse_delta()
{
    Window* window = g_objm.get_active_window();
    if (!window) return int2{};

    return {window->input.mouse_x_delta, window->input.mouse_y_delta};
}

int2 get_mouse_wheel_delta()
{
    Window* window = g_objm.get_active_window();
    if (!window) return int2{};

    return {window->input.mouse_wheel_x_delta, window->input.mouse_wheel_y_delta};
}

bool connect_to_joystick()
{
    Window* window = g_objm.get_active_window();
    if (!window) return false;

    if (SDL_NumJoysticks() < 1) {
        env_soft_error("Couldn't find any joysticks: %s", SDL_GetError());
        return false;
    }

    window->joystick = SDL_JoystickOpen(0);
    
    if (!window->joystick) {
        env_soft_error("Couldn't open the joystick: %s", SDL_GetError());
        return false;
    }
    return true;
}

bool is_connected_to_joystick()
{
    Window* window = g_objm.get_active_window();
    if (!window) return false;

    return window->joystick != nullptr;
}

bool disconnect_from_joystick()
{
    Window* window = g_objm.get_active_window();
    if (!window) return false;
    
    window->joystick = nullptr;

    return true;
}

int16_t get_joystick_axis_raw(uint8_t axis)
{
    if (axis >= ENV_MAX_JOYSTICK_AXES) {
        env_soft_error("Unable to set joystick range because the axis index '%d' is greater than the expected maximum '%d'", axis, ENV_MAX_JOYSTICK_AXES);
        return 0.0;
    }

    Window* window = g_objm.get_active_window();
    if (!window) return 0.0;

    return window->input.joystick_axes_raw[axis];
}

bool assign_joystick_axis_idx_to_axis_type(uint8_t axis_idx, Joystick_Axis axis_type)
{
    Window* window = g_objm.get_active_window();
    if (!window) return false;

    window->input.joystick_axis_type_to_idx[axis_type] = axis_idx;
    return true;
}

bool set_joystick_axis_range(Joystick_Axis axis_type, int16_t min, int16_t max, int16_t zero)
{
    Window* window = g_objm.get_active_window();
    if (!window) return false;

    window->input.joystick_axes_types_raw_min[axis_type] = min;
    window->input.joystick_axes_types_raw_max[axis_type] = max;
    window->input.joystick_axes_types_raw_zero[axis_type] = zero;
    return true;
}

// map a value from an input range to an output range
static double range_map(double x, double in_min, double in_max, double out_min, double out_max)
{
      return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

// We map the raw value to [-1.0, 1.0] based on the raw min max and zero values.
// Including zero ensures that the sticks resting position is actually at zero.
// On the downside, the output won't be smove in 0, because we have two slightly
// different linear interpolations for > 0 and < 0.
double get_joystick_axis_mapped_value(Joystick_Axis axis_type)
{
    Window* window = g_objm.get_active_window();
    if (!window) return 0.0;

    if (window->input.joystick_axes_types_raw_min[axis_type] == 0 || window->input.joystick_axes_types_raw_max[axis_type] == 0) {
        env_soft_error("The value range for the axis '%d' has not been specified", window->input.joystick_axis_type_to_idx[axis_type]);
        return 0.0;
    }

    int16_t raw = window->input.joystick_axes_raw[window->input.joystick_axis_type_to_idx[axis_type]];

    if (raw < window->input.joystick_axes_types_raw_zero[axis_type]) {
        return range_map((double)raw,
                         (double)window->input.joystick_axes_types_raw_min[axis_type],
                         (double)window->input.joystick_axes_types_raw_zero[axis_type],
                         -1.0, 0.0);
    }
    else {
        return range_map((double)raw,
                         (double)window->input.joystick_axes_types_raw_zero[axis_type],
                         (double)window->input.joystick_axes_types_raw_max[axis_type],
                         0.0, 1.0);
    }
}
