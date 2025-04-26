#include <math.hpp>
#include <utils/Entity.h>

namespace filament {
    class View;
    class Camera;
    class Renderer;
}

namespace fmt = filament;
namespace futils = utils;

struct Environment;

struct Camera {
    ~Camera();
    
    fmt::Camera* fcamera = nullptr;
    futils::Entity camera_fentity;
    fmt::View* view = nullptr;
    fmt::Renderer* renderer = nullptr;
    
    Environment* env = nullptr;
    double image_time_ms = 0; // when the image was rendered

    // state which is not saved by filament
    double3 up;
    double vertical_fov;
};

double update_image_time(Camera* camera);
void set_camera_image_size(Camera* camera, int width, int height);
uint32_t get_camera_image_width(Camera* camera);
uint32_t get_camera_image_height(Camera* camera);
