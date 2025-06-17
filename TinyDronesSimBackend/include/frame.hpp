#include "../tinydronessim_backend.hpp"
#include <math.hpp>

#include <backend/PixelBufferDescriptor.h>

#include <chrono>

class Environment;

namespace filament {
    class SwapChain;
    class View;
    class Renderer;
    class Fence;
}

namespace fmt = filament;
namespace fbackend = filament::backend;

class Camera;

class Frame {
public:
    static Frame_ID create(Environment_ID env_id,
                           Camera_ID camera_id,
                           uint32_t width,
                           uint32_t height,
                           fbackend::PixelDataFormat pixel_data_format,
                           fbackend::PixelDataType pixel_data_type);
    
    static Frame_ID create(Environment_ID env_id,
                           Camera_ID camera_id,
                           fmt::SwapChain* swap_chain,
                           uint32_t width,
                           uint32_t height);
    ~Frame();

    bool set_size(uint32_t width, uint32_t height);
    bool set_position(int32_t left, int32_t bottom);
    uint32_2 get_size();

    // copying the frame to the cpu memory (this is very slow)
    bool enable_pixel_capture(fbackend::PixelDataFormat pixel_data_format, fbackend::PixelDataType pixel_data_type);
    void disable_pixel_capture();
    bool pixel_capture_is_enabled() { return m_capture_pixels; }
    fbackend::PixelDataFormat get_pixel_data_format() { return m_pixel_data_format; }
    fbackend::PixelDataType get_pixel_data_type() { return m_pixel_data_type; }

    bool render(); // returns false if the gpu is already busy

    uint32_2 get_pixel_data(void** pixel_data);
    Environment_ID get_environment() { return m_env_id; }
    Camera_ID get_camera() { return m_camera_id; }
    double get_framerate_fps();

private:
    Frame() {};
    Frame_ID m_id;
    
    fmt::SwapChain* m_swap_chain = nullptr;
    bool m_swap_chain_internally_managed = false;
    
    Environment_ID m_env_id;
    Camera_ID m_camera_id;

    fmt::View* m_view = nullptr;
    fmt::Renderer* m_renderer = nullptr;

    bool m_capture_pixels = false;
    fbackend::PixelDataFormat m_pixel_data_format;
    fbackend::PixelDataType m_pixel_data_type;
    // fmt::Fence* m_pixel_capture_fence = nullptr;

    int32_t m_left = 0;
    int32_t m_bottom = 0;
    uint32_t m_width = 0;
    uint32_t m_height = 0;
    size_t m_pixel_data_size = 0;
    void* m_pixel_data = nullptr;

    uint64_t m_delta_time_between_renders_micro_s = 0;
    std::chrono::time_point<std::chrono::high_resolution_clock> m_time_point_prev_render;
};
