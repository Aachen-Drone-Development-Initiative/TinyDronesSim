#include "../tinydronessim_backend.hpp"
#include <camera.hpp>
#include <frame.hpp>

#include <cstdint>
#include <environment.hpp>
#include <object_manager.hpp>
#include <logging.hpp>
#include <math.hpp>

#include <filament/Engine.h>
#include <filament/Camera.h>
#include <filament/Renderer.h>
#include <filament/View.h>
#include <filament/Viewport.h>
#include <utils/EntityManager.h>
#include <backend/PixelBufferDescriptor.h>

Camera_ID Camera::create(Environment_ID env_id,
                         double3 pos,
                         double3 lookat,
                         double3 up,
                         double vertical_fov,
                         double aspect_ratio,
                         double near_plane,
                         double far_plane)
{
    Environment* env = g_objm.get_object(env_id);
    if (!env) return {TDS_INVALID_ID};
    
    Camera* camera = new Camera;
    
    camera->m_env_id = env_id;

    auto camera_result = env->create_filament_camera();
    camera->m_camera = camera_result.first;
    if (!camera->m_camera) {
        tds_hard_error("Failed to create Filament Camera");
    }
    camera->m_camera_entity = camera_result.second;

    camera->m_up = up;
    camera->m_camera->lookAt(d3_to_fd3(pos), d3_to_fd3(lookat), d3_to_fd3(camera->m_up));
    
    camera->m_vertical_fov = vertical_fov;
    camera->m_aspect_ratio = aspect_ratio;
    camera->m_near_plane = near_plane;
    camera->m_far_plane = far_plane;
    camera->m_camera->setProjection(camera->m_vertical_fov,
                                    camera->m_aspect_ratio,
                                    camera->m_near_plane,
                                    camera->m_far_plane,
                                    fmt::Camera::Fov::VERTICAL);

    return (camera->m_id = g_objm.add_object(camera));
}

TDS_API Camera_ID camera_create(Environment_ID env_id,
                                double3 pos,
                                double3 lookat,
                                double3 up,
                                double vertical_fov,
                                double aspect_ratio,
                                double near_plane,
                                double far_plane)
{
    return Camera::create(env_id, pos, lookat, up, vertical_fov, aspect_ratio, near_plane, far_plane);
}

void Camera::set_fov_vertical(double vertical_fov)
{
    m_vertical_fov = vertical_fov;
    m_camera->setProjection(vertical_fov, m_aspect_ratio, m_camera->getNear(), m_camera->getCullingFar(), fmt::Camera::Fov::VERTICAL);
}

TDS_API bool camera_set_fov_vertical(Camera_ID camera_id, double vertical_fov)
{
    Camera* camera = g_objm.get_object(camera_id);
    if (!camera) return false;

    camera->set_fov_vertical(vertical_fov);
    return true;
}

void Camera::set_aspect_ratio(double aspect_ratio) {
    m_aspect_ratio = aspect_ratio;
    m_camera->setProjection(m_vertical_fov, aspect_ratio, m_camera->getNear(), m_camera->getCullingFar(), fmt::Camera::Fov::VERTICAL);
}

TDS_API double camera_fov_vertical(Camera_ID camera_id)
{
    Camera* camera = g_objm.get_object(camera_id);
    if (!camera) return 0;
    return camera->m_vertical_fov;
}

TDS_API double3 camera_up_vector(Camera_ID camera_id)
{
    Camera* camera = g_objm.get_object(camera_id);
    if (!camera) return double3{};
    return camera->m_up;
}

TDS_API double3 camera_forward_vector(Camera_ID camera_id)
{
    Camera* camera = g_objm.get_object(camera_id);
    if (!camera) return double3{};
    return fd3_to_d3(camera->m_camera->getForwardVector());
}

TDS_API bool camera_set_position(Camera_ID camera_id, double3 pos)
{
    Camera* camera = g_objm.get_object(camera_id);
    if (!camera) return false;
    
    return entity_set_position(camera->m_camera_entity, pos);
}

TDS_API bool camera_set_orientation(Camera_ID camera_id, Quaternion orientation)
{
    Camera* camera = g_objm.get_object(camera_id);
    if (!camera) return false;
    
    return entity_set_orientation(camera->m_camera_entity, orientation);
}

TDS_API bool camera_set_position_and_orientation(Camera_ID camera_id, double3 pos, Quaternion orientation)
{
    Camera* camera = g_objm.get_object(camera_id);
    if (!camera) return false;
    
    return entity_set_position_and_orientation(camera->m_camera_entity, pos, orientation);
}
