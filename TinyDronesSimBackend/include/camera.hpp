#include "object_manager.hpp"
#include <math.hpp>

namespace filament {
    class View;
    class Camera;
    class Renderer;
}

namespace fmt = filament;

class Environment;

class Camera {
public:
    static Camera_ID create(Environment_ID,
                            double3 pos,
                            double3 lookat,
                            double3 up,
                            double vertical_fov,
                            double aspect_ratio,
                            double near_plane,
                            double far_plane);
    ~Camera() {};

    double update_image_time();
    void set_fov_vertical(double vertical_fov);
    void set_aspect_ratio(double aspect_ratio);

    fmt::Camera* get_filament_camera() { return m_camera; }

    Environment_ID get_environment() { return m_env_id; }

    friend bool camera_set_position(Camera_ID, double3 pos);
    friend bool camera_set_orientation(Camera_ID, Quaternion orientation);
    friend bool camera_set_position_and_orientation(Camera_ID, double3 pos, Quaternion orientation);
    friend double camera_fov_vertical(Camera_ID);
    friend double3 camera_up_vector(Camera_ID);
    friend double3 camera_forward_vector(Camera_ID);
    friend Entity_ID camera_filament_entity(Camera_ID);

private:
    Camera() {};
    Camera_ID m_id;

    fmt::Camera* m_camera = nullptr;
    Entity_ID m_camera_entity;
    
    Environment_ID m_env_id;
    double m_image_time_ms = 0; // when the image was rendered

    int2 m_image_size = {};
    double3 m_up = {};
    double m_vertical_fov = 0;
    double m_aspect_ratio = 0;
    double m_near_plane = 0;
    double m_far_plane = 0;
};
