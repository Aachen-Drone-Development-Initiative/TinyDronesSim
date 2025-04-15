#include "../environments.h"
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
    Camera* camera = state.get_as_camera(camera_id);
    if (!camera) return ENV_INVALID_UUID;

    Window* window = new Window;
    window->camera = camera;
    window->target_fps = target_fps;

    if (SDL_InitSubSystem(SDL_INIT_EVENTS) < 0) {
        env_hard_error("Failed to initialize SDL");
    }

    uint32_t sdl_window_flags = SDL_WINDOW_SHOWN
        | SDL_WINDOW_ALLOW_HIGHDPI
        | SDL_WINDOW_RESIZABLE;
    
    window->sdl_window = SDL_CreateWindow(name, 0, 0,
                                          __get_camera_image_width(camera),
                                          __get_camera_image_height(camera),
                                          sdl_window_flags);
    
    window->frame = __create_frame(window->camera->env,
                                   window->camera->env->engine->createSwapChain(
                                       get_native_window(window->sdl_window)));
    
    return state.add_object({window}).id;
}

Window::~Window()
{
    delete frame;
    SDL_DestroyWindow(sdl_window);
}

bool window_should_close(UUID window_id)
{
    Window* window = state.get_as_window(window_id);
    if (!window) return false;
    
    return window->should_close;
}

bool update_window(UUID window_id)
{
    Window* window = state.get_as_window(window_id);
    if (!window) return false;
    
    /*
     * Rendering
     *
     * Because the SDL-event response is handled outside (by the user) we
     * render first and collect events afterwards to reduce latency.
     */
    
    __render_frame(window->camera, window->frame);

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
    uint32_t sld_window_id = SDL_GetWindowID(window->sdl_window);
    SDL_Event event;

    while (SDL_PollEvent(&event) != 0) {
        switch (event.type) {
        case SDL_QUIT:
            window->should_close = true;
            break;
        case SDL_WINDOWEVENT:
            switch (event.window.event) {
            case SDL_WINDOWEVENT_RESIZED:
                if (event.window.windowID == sld_window_id) {
                    int width, height;
                    SDL_GL_GetDrawableSize(window->sdl_window, &width, &height);
                    __set_camera_image_size(window->camera, width, height);
                }
                break;
                
            case SDL_WINDOWEVENT_CLOSE:
                window->should_close = true;
                break;
                
            default:
                break;
            }
            break;
            
        case SDL_MOUSEMOTION:
            if (event.motion.windowID == sld_window_id) {
                window->input.mouse_x = event.motion.x;
                window->input.mouse_y = event.motion.y;
                window->input.mouse_x_delta = event.motion.xrel;
                window->input.mouse_y_delta = event.motion.yrel;
            }
            break;
            
        case SDL_MOUSEBUTTONDOWN:
            if (event.button.windowID == sld_window_id) {
                window->input.mouse_event_this_frame[event.button.button] = 1u;
            }
            break;

        case SDL_MOUSEBUTTONUP:
            if (event.button.windowID == sld_window_id) {
                window->input.mouse_event_this_frame[event.button.button] = 0u;
            }
            break;
            
        case SDL_MOUSEWHEEL:
            if (event.wheel.windowID == sld_window_id) {
                window->input.mouse_wheel_x_delta = event.wheel.x;
                window->input.mouse_wheel_y_delta = event.wheel.y;
            }
            break;
            
        case SDL_KEYDOWN:
            if (event.key.windowID == sld_window_id) {
                window->input.key_event_this_frame[event.key.keysym.scancode] = KEY_EVENT_DOWN;
            }
            break;
        
        case SDL_KEYUP:
            if (event.key.windowID == sld_window_id) {
                window->input.key_event_this_frame[event.key.keysym.scancode] = 0u;
            }
            break;
        
        default:
            break;
        }
    }

    /*
     * FPS correction
     * sleep so that 'target_fps' is never surpassed
     */
    
    double prev_image_time_ms = __update_image_time(window->camera);
    double time_delta_ms = window->camera->image_time_ms - prev_image_time_ms;
    double target_time_delta_ms = 1.0e3 / window->target_fps;
    window->last_frame_time_ms = time_delta_ms;
    if (time_delta_ms < target_time_delta_ms) {
        SDL_Delay(target_time_delta_ms - time_delta_ms);
        window->last_frame_time_ms = target_time_delta_ms;
    }
    
    return true;
}

double get_last_frame_time_ms(UUID window_id)
{
    Window* window = state.get_as_window(window_id);
    if (!window) return 0.0;
    
    return window->last_frame_time_ms;
}

bool is_key_down(UUID window_id, SDL_Scancode key)
{
    Window* window = state.get_as_window(window_id);
    if (!window) return false;
    
    return window->input.key_event_this_frame[key] & KEY_EVENT_DOWN;
}

bool is_key_pressed(UUID window_id, SDL_Scancode key)
{
    Window* window = state.get_as_window(window_id);
    if (!window) return false;
    
    return (window->input.key_event_this_frame[key] & KEY_EVENT_DOWN)
        && !(window->input.key_event_prev_frame[key] & KEY_EVENT_DOWN);
}

bool is_key_up(UUID window_id, SDL_Scancode key)
{
    Window* window = state.get_as_window(window_id);
    if (!window) return false;

    return !(window->input.key_event_this_frame[key] & KEY_EVENT_DOWN);
}

bool is_key_released(UUID window_id, SDL_Scancode key)
{
    Window* window = state.get_as_window(window_id);
    if (!window) return false;

    return !(window->input.key_event_this_frame[key] & KEY_EVENT_DOWN)
        && (window->input.key_event_prev_frame[key] & KEY_EVENT_DOWN);
}

bool is_mouse_button_down(UUID window_id, Mouse_Button button)
{
    Window* window = state.get_as_window(window_id);
    if (!window) return false;

    return window->input.mouse_event_this_frame[button];
}

bool is_mouse_button_pressed(UUID window_id, Mouse_Button button)
{
    Window* window = state.get_as_window(window_id);
    if (!window) return false;

    return window->input.mouse_event_this_frame[button]
        && !(window->input.mouse_event_prev_frame[button]);
}

bool is_mouse_button_up(UUID window_id, Mouse_Button button)
{
    Window* window = state.get_as_window(window_id);
    if (!window) return false;

    return !window->input.mouse_event_this_frame[button];
}

bool is_mouse_button_released(UUID window_id, Mouse_Button button)
{
    Window* window = state.get_as_window(window_id);
    if (!window) return false;

    return !(window->input.mouse_event_this_frame[button])
        && window->input.mouse_event_prev_frame[button];    
}
    
int2 get_mouse_pos(UUID window_id)
{
    Window* window = state.get_as_window(window_id);
    if (!window) return int2{};

    return {window->input.mouse_x, window->input.mouse_y};
}

int2 get_mouse_delta(UUID window_id)
{
    Window* window = state.get_as_window(window_id);
    if (!window) return int2{};

    return {window->input.mouse_x_delta, window->input.mouse_y_delta};
}

int2 get_mouse_wheel_delta(UUID window_id)
{
    Window* window = state.get_as_window(window_id);
    if (!window) return int2{};

    return {window->input.mouse_wheel_x_delta, window->input.mouse_wheel_y_delta};
}
