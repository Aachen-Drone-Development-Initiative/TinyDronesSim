
// Building
// g++ x11_filament.cpp -o x11_filament  -lX11 -lGL 

#include "filament/Camera.h"
#include "filament/SwapChain.h"
#include "math/vec3.h"
#include <SDL2/SDL_video.h>
#include <iostream>

#include <SDL2/SDL.h>
#include <SDL2/SDL_syswm.h>

// #include <X11/Xlib.h>
// #include <GL/glx.h>
#include <EGL/egl.h>

#include <filament/Engine.h>
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

#include <stb_image.h>

namespace fmath = filament::math;
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

static void* get_native_window(SDL_Window* sdlWindow) {
    SDL_SysWMinfo wmi;
    SDL_VERSION(&wmi.version);
    if(!SDL_GetWindowWMInfo(sdlWindow, &wmi)) {
        std::cout << "SDL version not supported.\n";
    }
    if (wmi.subsystem == SDL_SYSWM_X11) {
        std::cout << "Using x11\n";
        Window win = (Window) wmi.info.x11.window;
        return (void *) win;
    }
    else if (wmi.subsystem == SDL_SYSWM_WAYLAND) {
        std::cout << "Using wayland";
        int width = 0;
        int height = 0;
        SDL_GetWindowSize(sdlWindow, &width, &height);

        // Static is used here to allocate the struct pointer for the lifetime of the program.
        // Without static the valid struct quickyly goes out of scope, and ends with seemingly
        // random segfaults.
        static struct {
            struct wl_display *display;
            struct wl_surface *surface;
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

void resize_window(SDL_Window* sdl_window, fmt::View* view, fmt::Camera* camera, fmt::Engine* engine)
{
    int width, height;
    SDL_GL_GetDrawableSize(sdl_window, &width, &height);
    view->setViewport({0, 0, uint32_t(width), uint32_t(height)});
    camera->setProjection(60.0, double(width) / double(height), 0.1, 50.0);
}

int main(int argc, char** argv) {

    SDL_Init(SDL_INIT_EVENTS);

    int width = 800, height = 600;
    SDL_Window* sdl_window = SDL_CreateWindow("sdl filament test", 0, 0, width, height, SDL_WINDOW_RESIZABLE);

    fmt::Engine* engine = fmt::Engine::Builder().backend(fmt::Engine::Backend::OPENGL).build();
    
    if (!engine) {
        std::cout << "ERROR: Failed to create Engine.\n";
        return 1;
    }

    fmt::Renderer* renderer = engine->createRenderer();
    fmt::SwapChain* swap_chain = engine->createSwapChain(get_native_window(sdl_window));

    utils::Entity camera_entity;
    utils::EntityManager& em = utils::EntityManager::get();
    em.create(1, &camera_entity);
    fmt::Camera* camera = engine->createCamera(camera_entity);

    fmt::View* view = engine->createView();
    view->setCamera(camera);

    fmt::Scene* scene = engine->createScene();
    view->setScene(scene);

    // fmt::Skybox* skybox = fmt::Skybox::Builder().color({0, 1, 0, 0}).build(*engine);
    // scene->setSkybox(skybox);

    add_ibl_skybox({"./assets/rogland_sunset_2k.hdr"}, engine, scene);

    resize_window(sdl_window, view, camera, engine);

    SDL_Event event;
    bool quit = false;

    while (!quit) {
        while (SDL_PollEvent(&event) != 0) {
            switch (event.type) {
            case SDL_QUIT:
                quit = true;
                break;
            case SDL_WINDOWEVENT:
                switch (event.window.event) {
                case SDL_WINDOWEVENT_RESIZED:
                    if (event.window.windowID == SDL_GetWindowID(sdl_window)) {
                        resize_window(sdl_window, view, camera, engine);
                        break;
                    }
                    break;
                case SDL_WINDOWEVENT_CLOSE:
                    if (event.window.windowID == SDL_GetWindowID(sdl_window)) {
                        SDL_HideWindow(sdl_window);
                        break;
                    }
                    break;
                default:
                    break;
                }
                break;
            default:
                break;
            }

            if (renderer->beginFrame(swap_chain)) {
                renderer->render(view);
                renderer->endFrame();
            }
        
            SDL_Delay(16);
        }
    }
    
    return 0;
}
