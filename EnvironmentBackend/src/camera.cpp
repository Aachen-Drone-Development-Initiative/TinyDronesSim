#include "../environments.hpp"
#include <camera.hpp>

#include <environment.hpp>
#include <object_manager.hpp>
#include <logging.hpp>
#include <math.hpp>

#include <filament/Engine.h>
#include <filament/Camera.h>
#include <filament/View.h>
#include <filament/Viewport.h>
#include <utils/EntityManager.h>

#include <SDL.h>

namespace futils = utils;

Camera_ID create_camera(Environment_ID env_id, double3 pos, double3 lookat, double3 up, double vertical_fov, double near_plane, double far_plane, uint32_t width, uint32_t height)
{
    Environment* env = g_objm.get_object(env_id);
    if (!env) return {ENV_INVALID_UUID};

    Camera* camera = new Camera;
    camera->env = env;
    futils::EntityManager& entity_m = futils::EntityManager::get();
    camera->camera_fentity = g_objm.add_object({entity_m.create(), env});
    camera->fcamera = env->engine->createCamera(g_objm.get_object(camera->camera_fentity).entity);
    camera->up = up;
    camera->fcamera->lookAt(d3_to_fd3(pos), d3_to_fd3(lookat), d3_to_fd3(up));
    camera->vertical_fov = vertical_fov;
    camera->fcamera->setProjection(vertical_fov,
                                   double(width) / double(height),
                                   near_plane, far_plane,
                                   fmt::Camera::Fov::VERTICAL);
    
    camera->view = env->engine->createView();
    camera->view->setCamera(camera->fcamera);
    assert(env->scene != nullptr);
    camera->view->setScene(env->scene);
    camera->view->setViewport({0, 0, width, height});

    camera->renderer = env->engine->createRenderer();

    return g_objm.add_object(camera);
}

Camera::~Camera()
{
    fmt::Engine* engine = env->engine;
    engine->destroy(view);

    // FIXME: WE SHOULD DESTROY THE RENDERER, BUT FILAMENT CRASHES HERE SOMETIMES
    // engine->destroy(renderer);
}

// Environment* get_camera_environment(Camera* camera) { return camera->env; }
uint32_t get_camera_image_width(Camera* camera) { return camera->view->getViewport().width; }
uint32_t get_camera_image_height(Camera* camera) { return camera->view->getViewport().height; }

bool set_camera_fov_vertical(Camera_ID camera_id, double vertical_fov)
{
    Camera* camera = g_objm.get_object(camera_id);
    if (!camera) return false;
    
    camera->vertical_fov = vertical_fov;
    camera->fcamera->setProjection(vertical_fov, double(camera->view->getViewport().width) / double(camera->view->getViewport().height),
                                   camera->fcamera->getNear(),
                                   camera->fcamera->getCullingFar(),
                                   fmt::Camera::Fov::VERTICAL);
    return true;
}

double get_camera_fov_vertical(Camera_ID camera_id)
{
    Camera* camera = g_objm.get_object(camera_id);
    if (!camera) return 0;
    return camera->vertical_fov;
}

double3 get_camera_up_vector(Camera_ID camera_id)
{
    Camera* camera = g_objm.get_object(camera_id);
    if (!camera) return double3{};
    return camera->up;
}

double3 get_camera_forward_vector(Camera_ID camera_id)
{
    Camera* camera = g_objm.get_object(camera_id);
    if (!camera) return double3{};
    return fd3_to_d3(camera->fcamera->getForwardVector());
}

void set_camera_image_size(Camera* camera, int width, int height)
{
    camera->view->setViewport({0, 0, uint32_t(width), uint32_t(height)});
    float fov = camera->fcamera->getFieldOfViewInDegrees(fmt::Camera::Fov::VERTICAL);
    camera->fcamera->setProjection(fov, double(width) / double(height),
                                   camera->fcamera->getNear(),
                                   camera->fcamera->getCullingFar(),
                                   fmt::Camera::Fov::VERTICAL);
}

double update_image_time(Camera* camera)
{
    static double sdl_ticks_per_ms = double(SDL_GetPerformanceFrequency()) * 10e-3;
    double prev_time = camera->image_time_ms;
    camera->image_time_ms = double(SDL_GetPerformanceCounter()) / sdl_ticks_per_ms;
    return prev_time;
}

ENV_API Filament_Entity_ID get_camera_filament_entity(Camera_ID camera_id)
{
    Camera* camera = g_objm.get_object(camera_id);
    if (!camera) return {};
    
    return camera->camera_fentity;
}
