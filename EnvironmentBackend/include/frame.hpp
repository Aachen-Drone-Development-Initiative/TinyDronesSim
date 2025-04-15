#include <math.hpp>

#include <backend/DriverEnums.h>

struct Environment;

namespace filament {
    class SwapChain;
}

namespace fmt = filament;

struct Camera;

struct Frame {
    ~Frame();
    
    fmt::SwapChain* swap_chain = nullptr;
    Environment* env;

    // flag for copying the frame to the cpu memory (this is very slow)
    bool capture_pixels = false;
    filament::backend::PixelDataFormat pixel_data_format;
    filament::backend::PixelDataType pixel_data_type;
    // width, height and pixel_data_size will be set for you, when a frame is saved in cpu memory
    uint32_t width = 0;
    uint32_t height = 0;
    size_t pixel_data_size = 0;
    void* pixel_data = nullptr;
};

Frame* __create_frame(Environment* env, fmt::SwapChain* swap_chain);
bool __render_frame(Camera* camera, Frame* frame);
