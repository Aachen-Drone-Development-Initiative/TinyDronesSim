
// #include <X11/Xlib.h>
// #include <X11/Xutil.h>
// #include <X11/keysymdef.h>

// #include <GL/glx.h>
#include "filament/SwapChain.h"
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

// #include <stdio.h>

#define UI_LINUX
#define UI_IMPLEMENTATION
#include "../extern/luigi/luigi2.h"

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

struct Render_State {
    fmt::Engine* engine;
    fmt::Renderer* renderer;
    fmt::SwapChain* swap_chain;
    fmt::View* view;
    fmt::Camera* camera;
};

void render_to_managed_window(void* callback_state, UIWindow* window, size_t width, size_t height)
{
    Render_State* rs = (Render_State*) callback_state;

    rs->view->setViewport({0, 0, (uint32_t) width, (uint32_t) height});
    rs->camera->setProjection(60.0, double(width) / double(height), 0.1, 50.0);
    
    if (rs->renderer->beginFrame(rs->swap_chain)) {
        rs->renderer->render(rs->view);
        rs->renderer->endFrame();
        fmt::Fence::waitAndDestroy(rs->engine->createFence());
    }
}

int main(int argc, char** argv)
{
    UIInitialise();

    utils::EntityManager& em = utils::EntityManager::get();

    UIWindow* ui_window = UIWindowCreate(0, 0, "uimdi", 1200, 800);

    UISplitPane* v_split = UISplitPaneCreate(&ui_window->e, UI_SPLIT_PANE_VERTICAL, 0.9);
	
    // UIPanel *upper_panel = UIPanelCreate(&v_split->e, UI_PANEL_COLOR_1);
    // UITabPane* upper_tab_pane = UITabPaneCreate(&v_split->e, 0, "hello sussy baka\tbeatiful");

    Render_State render_state_1{};
    render_state_1.engine = fmt::Engine::Builder().backend(fmt::Engine::Backend::VULKAN).build();
    render_state_1.renderer = render_state_1.engine->createRenderer();
    render_state_1.view = render_state_1.engine->createView();

    // UIWindow* ui_sub_window = UIWindowCreate(ui_window, UI_WINDOW_PARENT_MANAGED_SUBWINDOW, "", 300, 200);

    UIViewportFrame* vp_frame = UIViewportFrameCreate(&v_split->e, UI_VIEWPORT_FRAME_PARENT_MANAGED_WINDOW_MODE, 300, 200);
    vp_frame->callback_state = (void*) &render_state_1;
    vp_frame->render_to_managed_window = render_to_managed_window;
    render_state_1.swap_chain = render_state_1.engine->createSwapChain((void*) vp_frame->window->window);

    utils::Entity camera_entity;

    em.create(1, &camera_entity);
    render_state_1.camera = render_state_1.engine->createCamera(camera_entity);
    render_state_1.view->setCamera(render_state_1.camera);

    fmt::Scene* scene = render_state_1.engine->createScene();
    render_state_1.view->setScene(scene);

    // fmt::Skybox* skybox = fmt::Skybox::Builder().color({0, 1, 0, 0}).build(*render_state_1.engine);
    // scene->setSkybox(skybox);
    add_ibl_skybox({"./assets/rogland_sunset_2k.hdr"}, render_state_1.engine, scene);

    
    // Render_State render_state_2{};
    // render_state_2.engine = fmt::Engine::Builder().backend(fmt::Engine::Backend::VULKAN).build();
    // render_state_2.renderer = render_state_2.engine->createRenderer();
    // render_state_2.view = render_state_2.engine->createView();

    // UIViewportFrame* vp_frame_2 = UIViewportFrameCreate(&upper_tab_pane->e, UI_VIEWPORT_FRAME_PARENT_MANAGED_WINDOW_MODE, 300, 200);
    // vp_frame_2->callback_state = (void*) &render_state_2;
    // vp_frame_2->render_to_managed_window = render_to_managed_window;
    // render_state_2.swap_chain = render_state_2.engine->createSwapChain((void*) vp_frame_2->window->window);
    
    // utils::Entity camera_entity_2;
    // em.create(1, &camera_entity_2);
    // render_state_2.camera = render_state_2.engine->createCamera(camera_entity_2);

    // render_state_2.view->setCamera(render_state_2.camera);

    // fmt::Scene* scene_2 = render_state_2.engine->createScene();
    // render_state_2.view->setScene(scene_2);

    // // fmt::Skybox* skybox_2 = fmt::Skybox::Builder().color({0, 1, 0, 0}).build(*render_state_2.engine);
    // // scene_2->setSkybox(skybox);
    // add_ibl_skybox({"./assets/rogland_sunset_2k.hdr"}, render_state_2.engine, scene_2);

    UIPanel *lower_panel = UIPanelCreate(&v_split->e, UI_PANEL_COLOR_1);

    return UIMessageLoop();
}
