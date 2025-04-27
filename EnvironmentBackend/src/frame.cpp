#include "../environments.hpp"
#include <frame.hpp>

#include <camera.hpp>
#include <environment.hpp>
#include <object_manager.hpp>
#include <logging.hpp>

#include <filament/Renderer.h>
#include <filament/Engine.h>
#include <backend/PixelBufferDescriptor.h>

ENV_API Frame_ID create_frame(Environment_ID env_id, uint32_t width, uint32_t height)
{
    Environment* env = g_objm.get_object(env_id);
    if (!env) return {ENV_INVALID_UUID};

    Frame* frame = new Frame;
    frame->env = env;
    frame->swap_chain = frame->env->engine->createSwapChain(width, height);
    return g_objm.add_object(frame);
}

Frame* create_frame(Environment* env, fmt::SwapChain* swap_chain)
{
    assert(env);
    Frame* frame = new Frame;
    frame->swap_chain = swap_chain;
    frame->env = env;
    return frame;
}

Frame::~Frame()
{
    env->engine->destroy(swap_chain);
}

ENV_API bool enable_pixel_capture(Frame_ID frame_id, filament::backend::PixelDataFormat pixel_data_format, filament::backend::PixelDataType pixel_data_type)
{
    Frame* frame = g_objm.get_object(frame_id);
    if (!frame) return false;

    frame->capture_pixels = true;
    frame->pixel_data_format = pixel_data_format;
    frame->pixel_data_type = pixel_data_type;
    return true;
}

ENV_API bool get_pixel_data(Frame_ID frame_id, void** pixel_data, uint32_t* width, uint32_t* height)
{
    Frame* frame = g_objm.get_object(frame_id);
    if (!frame) return false;

    if (!frame->capture_pixels) {
        env_soft_error("Can't get pixel data, because the pixels are not"
                       " being captured, call 'enable_pixel_capture' first");
        return false;
    }
    *pixel_data = frame->pixel_data;
    *width = frame->width;
    *height = frame->height;
    return *pixel_data != nullptr;
}

ENV_API bool disable_pixel_capture(Frame_ID frame_id)
{
    Frame* frame = g_objm.get_object(frame_id);
    if (!frame) return false;

    frame->capture_pixels = false;
    return true;
}

bool render_frame(Camera* camera, Frame* frame)
{
    // beginFrame() returns false if we need to skip a frame (gpu too busy)
    if (camera->renderer->beginFrame(frame->swap_chain)) {
        
        camera->renderer->render(camera->view);
        
        if (frame->capture_pixels) {

            frame->width = get_camera_image_width(camera);
            frame->height = get_camera_image_height(camera);
            size_t new_pixel_data_size = filament::backend::PixelBufferDescriptor::computeDataSize(
                frame->pixel_data_format,
                frame->pixel_data_type,
                frame->width, frame->height, 1);
            
            if (frame->pixel_data_size != new_pixel_data_size) {
                free(frame->pixel_data);
                frame->pixel_data = nullptr;
            }
            
            if (!frame->pixel_data) {
                frame->pixel_data = malloc(frame->pixel_data_size);
            }
            
            filament::backend::PixelBufferDescriptor pixel_buffer(
                frame->pixel_data,
                frame->pixel_data_size,
                frame->pixel_data_format,
                frame->pixel_data_type);

            camera->renderer->readPixels(0, 0, frame->width, frame->height, std::move(pixel_buffer));
        }
        
        camera->renderer->endFrame();
        return true;
    }
    return false;
}

ENV_API bool render_frame(Camera_ID camera_id, Frame_ID frame_id)
{
    Camera* camera = g_objm.get_object(camera_id);
    Frame* frame = g_objm.get_object(frame_id);
    if (!camera || !frame) return false;
    
    return render_frame(camera, frame);
}
