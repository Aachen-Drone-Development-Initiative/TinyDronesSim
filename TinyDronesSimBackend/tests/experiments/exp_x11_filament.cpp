
#include "filament/SwapChain.h"
#include "math/vec3.h"
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/keysymdef.h>

// #include <GL/glx.h>
#include <cstdint>
#include <iostream>

#include <SDL2/SDL.h>
#include <SDL2/SDL_events.h>
#include <SDL2/SDL_video.h>
#include <SDL2/SDL_syswm.h>

#include <filament/Engine.h>
#include <filament/Camera.h>
#include <filament/Scene.h>
#include <filament/Texture.h>
#include <filament/RenderTarget.h>
#include <filament/Viewport.h>
#include <filament/View.h>
#include <filament/Renderer.h>
#include <filament-iblprefilter/IBLPrefilterContext.h>
#include <filament/IndirectLight.h>
#include <filament/Skybox.h>
#include <utils/Path.h>
#include <math/norm.h>
#include <math/mat3.h>
#include <filament/IndexBuffer.h>
#include <filament/VertexBuffer.h>
#include <utils/EntityManager.h>
#include <filament/TransformManager.h>
#include <filament/Fence.h>

#include <stb_image.h>

#include <thread>
#include <chrono>

namespace fmt = filament;

bool add_ibl_skybox(utils::Path file_path, filament::Engine* engine, filament::Scene* scene)
{
    int width, height, n_channels;
    stbi_info(file_path.getAbsolutePath().c_str(), &width, &height, nullptr);
    // load image as float
    size_t size = width * height * sizeof(filament::math::float3);
    filament::math::float3* data = (filament::math::float3*)stbi_loadf(file_path.getAbsolutePath().c_str(), &width, &height, &n_channels, 3);
    filament::Texture::PixelBufferDescriptor::Callback destroy_callback = [](void* data, size_t, void*) {
        stbi_image_free(data);
    };

    if (data == nullptr || n_channels != 3) {
        std::cout << "Could not decode image: " << file_path.c_str() << '\n';
        destroy_callback(data, size, nullptr);
        return false;
    }

    if (width != height * 2) {
        std::cout << "Not an equirectangular image: " << file_path.c_str() << '\n';
        destroy_callback(data, size, nullptr);
        return false;
    }

    // load the texture
    filament::Texture::PixelBufferDescriptor buffer(
        data, size, filament::Texture::Format::RGB, filament::Texture::Type::FLOAT, destroy_callback, nullptr);
    
    filament::Texture* equirect = filament::Texture::Builder()
        .width((uint32_t)width)
        .height((uint32_t)height)
        .levels(0xff)
        .format(filament::Texture::InternalFormat::R11F_G11F_B10F)
        .sampler(filament::Texture::Sampler::SAMPLER_2D)
        .build(*engine);

    equirect->setImage(*engine, 0, std::move(buffer));

    IBLPrefilterContext context(*engine);
    IBLPrefilterContext::EquirectangularToCubemap equirectangularToCubemap(context);
    IBLPrefilterContext::SpecularFilter specularFilter(context);
    IBLPrefilterContext::IrradianceFilter irradianceFilter(context);

    filament::Texture* skybox_texture = equirectangularToCubemap(equirect);
    filament::Texture* texture = specularFilter(skybox_texture);
    
    filament::IndirectLight* indirect_light = filament::IndirectLight::Builder()
        .reflections(texture)
        .intensity(20000.0f)
        .build(*engine);

    filament::Skybox* skybox = filament::Skybox::Builder()
            .environment(skybox_texture)
            .showSun(true)
            .build(*engine);

    scene->setIndirectLight(indirect_light);
    scene->setSkybox(skybox);

    return true;
}

void resize_window(Display* display, Window window, fmt::SwapChain** swap_chain, fmt::View* view, fmt::Camera* camera, fmt::Engine* engine)
{
    XWindowAttributes attribs;
    XGetWindowAttributes(display, window, &attribs);

    engine->destroy(*swap_chain);
    *swap_chain = engine->createSwapChain((void*) window);
    
    view->setViewport({0, 0, (uint32_t) attribs.width, (uint32_t) attribs.height});
    camera->setProjection(60.0, double(attribs.width) / double(attribs.height), 0.1, 50.0);
}

int main(int argc, char** argv)
{
    Display *display;
    int screen_id;
    Window window;
    Screen* screen;
    GC gc;
    XEvent ev;

    display=XOpenDisplay((char *)0);
    screen_id=DefaultScreen(display);
    
    window=XCreateSimpleWindow(display,DefaultRootWindow(display),0,0,	
                               1200, 800, 5, WhitePixel(display,screen_id), BlackPixel(display,screen_id));

    XSetWindowBackgroundPixmap(display, window, None);

    /* here is where some properties of the window can be set.
       The third and fourth items indicate the name which appears
       at the top of the window and the name of the minimized window
       respectively.
    */
    XSetStandardProperties(display, window, "My Window", "HI!", None, NULL, 0, NULL);

    /* this routine determines which types of input are allowed in
       the input.  see the appropriate section for details...
    */
    XSelectInput(display, window, ExposureMask|ButtonPressMask|KeyPressMask);

    /* create the Graphics Context */
    gc=XCreateGC(display, window, 0,0);        

    /* clear the window and bring it on top of the other windows */
    XClearWindow(display, window);
    XMapRaised(display, window);
    	
    fmt::Engine* engine = fmt::Engine::Builder().backend(fmt::Engine::Backend::VULKAN).build();
    
    if (!engine) {
        std::cout << "ERROR: Failed to create Engine.\n";
        return 1;
    }

    fmt::Renderer* renderer = engine->createRenderer();
    fmt::SwapChain* swap_chain = engine->createSwapChain((void*)window);

    utils::Entity camera_entity;
    utils::EntityManager& em = utils::EntityManager::get();
    em.create(1, &camera_entity);
    fmt::Camera* camera = engine->createCamera(camera_entity);

    fmt::View* view = engine->createView();
    view->setCamera(camera);

    fmt::Scene* scene = engine->createScene();
    view->setScene(scene);

    fmt::Skybox* skybox = fmt::Skybox::Builder().color({0, 1, 0, 0}).build(*engine);
    scene->setSkybox(skybox);
    // add_ibl_skybox({"./assets/rogland_sunset_2k.hdr"}, engine, scene);

    resize_window(display, window, &swap_chain, view, camera, engine);
    
    while(true)
    {

        if (XPending(display)) {
            XNextEvent(display, &ev);
            if (ev.type == Expose) {
                resize_window(display, window, &swap_chain, view, camera, engine);
            }
        } else {
            // fmt::Fence::waitAndDestroy(engine->createFence());
            // XClearWindow(display, window);
            if (renderer->beginFrame(swap_chain)) {
                renderer->render(view);
                renderer->endFrame();
                // engine->flushAndWait();
                // fmt::Fence::waitAndDestroy(engine->createFence());
                printf("update\n");
            }
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(2));
    }

    XFreeGC(display, gc);
    XDestroyWindow(display, window);
    XCloseDisplay(display);

    return 0;
}
