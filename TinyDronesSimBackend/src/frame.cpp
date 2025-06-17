#include <frame.hpp>

#include <camera.hpp>
#include <environment.hpp>
#include <object_manager.hpp>
#include <logging.hpp>

#include <filament/Renderer.h>
#include <filament/Engine.h>
#include <filament/View.h>
#include <filament/Viewport.h>
#include <filament/Fence.h>
#include <utils/EntityManager.h>
// #include <backend/PixelBufferDescriptor.h>

Frame_ID Frame::create(Environment_ID env_id,
                       Camera_ID camera_id,
                       uint32_t width,
                       uint32_t height,
                       fbackend::PixelDataFormat pixel_data_format,
                       fbackend::PixelDataType pixel_data_type)
{
    Frame_ID frame_id = create(env_id, camera_id, nullptr, width, height);
    
    if (frame_id != TDS_INVALID_ID) {
        Frame* frame = g_objm.get_object(frame_id);
        assert(frame);

        Environment* env = g_objm.get_object(env_id);
        assert(env);

        frame->m_swap_chain = env->create_swap_chain(width, height);
        frame->m_swap_chain_internally_managed = true;
    
        if (!frame->m_swap_chain) {
            tds_hard_error("Failed to create Filament Swap Chain.");
        }
        // There is no point in not rendering to a window and also not copying the frame to cpu memory.
        frame->enable_pixel_capture(pixel_data_format, pixel_data_type);
    }
    return frame_id;
}

TDS_API Frame_ID frame_create(Environment_ID env_id, Camera_ID camera_id, uint32_t width, uint32_t height)
{
    return Frame::create(env_id, camera_id, width, height, fbackend::PixelDataFormat::RGB_INTEGER, fbackend::PixelDataType::UBYTE);
}

Frame_ID Frame::create(Environment_ID env_id,
                             Camera_ID camera_id,
                             fmt::SwapChain* swap_chain,
                             uint32_t width,
                             uint32_t height)
{
    Environment* env = g_objm.get_object(env_id);
    if (!env) return {TDS_INVALID_ID};

    Camera* camera = g_objm.get_object(camera_id);
    if (!camera) return {TDS_INVALID_ID};

    Frame* frame = new Frame;
    
    frame->m_env_id = env_id;
    frame->m_camera_id = camera_id;
    frame->m_width = width;
    frame->m_height = height;
    frame->m_swap_chain = swap_chain;
    
    frame->m_view = env->create_filament_view();
    if (!frame->m_view) {
        tds_soft_error("Failed to create a Filament View.");
        delete frame;
    }
    frame->m_view->setCamera(camera->get_filament_camera());
    frame->m_view->setViewport({0, 0, width, height});

    frame->m_renderer = env->create_filament_renderer();
    // fmt::Renderer::FrameRateOptions options = {
    //     float headRoomRatio = 0.0f;        //!< additional headroom for the GPU
    //     float scaleRate = 1.0f / 8.0f;     //!< rate at which the system reacts to load changes
    //     uint8_t history = 15;              //!< history size
    //     uint8_t interval = 1;              //!< desired frame interval in unit of 1.0 / DisplayInfo::refreshRate
    // };

    // FrameRateOptions options = {};
    // frame->m_renderer->setFrameRateOptions(FrameRateOptions const& options) noexcept;
    if (!frame->m_renderer) {
        tds_soft_error("Failed to create Filament Renderer.");
        delete frame;
    }

    return (frame->m_id = g_objm.add_object(frame));
}

Frame::~Frame()
{
    Environment* env = g_objm.get_object(m_env_id);
    if (env) {
        env->destroy_swap_chain(m_swap_chain);
        env->destroy_view(m_view);
        // FIXME: WE SHOULD DESTROY THE RENDERER, BUT FILAMENT CRASHES HERE SOMETIMES
        // env->destroy(m_renderer);
    }
    
    if (m_pixel_data) {
        free(m_pixel_data);
    }
}

uint32_2 Frame::get_size()
{
    fmt::Viewport viewport = m_view->getViewport();
    return { viewport.width, viewport.height };
}

TDS_API uint32_2 frame_size(Frame_ID frame_id)
{
    Frame* frame = g_objm.get_object(frame_id);
    if (!frame) return {};

    return frame->get_size();
}

bool Frame::set_size(uint32_t width, uint32_t height)
{
    if (m_width == width && m_height == height) return true;

    // Environment* env = g_objm.get_object(m_env_id);
    // if (!env) return false;
    
    Camera* camera = g_objm.get_object(m_camera_id);
    if (!camera) return false;

    m_width = width;
    m_height = height;
    
    // if (m_swap_chain_internally_managed) {
    //     env->destroy_swap_chain(m_swap_chain);
    //     m_swap_chain = env->create_swap_chain(m_width, m_height);
    // }

    m_view->setViewport(fmt::Viewport{m_left, m_bottom, m_width, m_height});
    camera->set_aspect_ratio(double(m_width) / double(m_height));

    // fmt::Fence::waitAndDestroy(env->create_fence());
    
    return true;
}

TDS_API bool frame_set_size(Frame_ID frame_id, uint32_t width, uint32_t height)
{
    Frame* frame = g_objm.get_object(frame_id);
    if (!frame) return false;

    frame->set_size(width, height);
    return true;
}

bool Frame::set_position(int32_t left, int32_t bottom)
{
    if (m_swap_chain_internally_managed) return false;
    
    int32_t m_left = left;
    int32_t m_bottom = bottom;
    m_view->setViewport(fmt::Viewport{m_left, m_bottom, m_width, m_height});
    return true;
}

bool Frame::enable_pixel_capture(fbackend::PixelDataFormat pixel_data_format, fbackend::PixelDataType pixel_data_type)
{
        
    m_capture_pixels = true;
    m_pixel_data_format = pixel_data_format;
    m_pixel_data_type = pixel_data_type;
    
    return true;
}

TDS_API bool frame_enable_pixel_capture(Frame_ID frame_id, fbackend::PixelDataFormat pixel_data_format, fbackend::PixelDataType pixel_data_type)
{
    Frame* frame = g_objm.get_object(frame_id);
    if (!frame) return false;

    frame->enable_pixel_capture(pixel_data_format, pixel_data_type);
    return true;
}

void Frame::disable_pixel_capture() { m_capture_pixels = false; }

TDS_API bool frame_disable_pixel_capture(Frame_ID frame_id)
{
    Frame* frame = g_objm.get_object(frame_id);
    if (!frame) return false;

    frame->disable_pixel_capture();
    return true;
}

uint32_2 Frame::get_pixel_data(void** pixel_data)
{
    if (!m_capture_pixels) {
        tds_soft_error("Can't get pixel data, because the pixels are not being captured, call 'enable_pixel_capture' first");
        return {};
    }
    
    *pixel_data = m_pixel_data;
    return {m_width, m_height};
}

TDS_API uint32_2 frame_get_pixel_data(Frame_ID frame_id, void** pixel_data)
{
    Frame* frame = g_objm.get_object(frame_id);
    if (!frame) return {};

    return frame->get_pixel_data(pixel_data);
}

bool Frame::render()
{
    // beginFrame() returns false if we need to skip a frame (gpu too busy)
    if (m_renderer->beginFrame(m_swap_chain))
    {
        m_renderer->render(m_view);

        if (m_capture_pixels)
        {
            size_t new_pixel_data_size = fbackend::PixelBufferDescriptor::computeDataSize(m_pixel_data_format, m_pixel_data_type, m_width, m_height, 1);

            if (m_pixel_data_size != new_pixel_data_size) {
                free(m_pixel_data);
                m_pixel_data_size = new_pixel_data_size;
                m_pixel_data = malloc(m_pixel_data_size);
            }

            filament::backend::PixelBufferDescriptor pixel_buffer(m_pixel_data, m_pixel_data_size, m_pixel_data_format, m_pixel_data_type);
            m_renderer->readPixels(0, 0, m_width, m_height, std::move(pixel_buffer));
        }
        
        m_renderer->endFrame();
        
        return true;
    }
    return false;
}
