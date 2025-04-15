#pragma once

#include <math.hpp>

#include <cstdint>

#include <backend/DriverEnums.h>

#include <SDL_scancode.h>
#include <SDL_mouse.h>

#define ENV_API __attribute__((visibility("default")))

extern "C" {

typedef uint64_t UUID;

/*
 * General Object Management
 */

ENV_API bool destroy(UUID obj_id);
ENV_API bool destroy_everything();

/*
 * Environment Handling
 *
 * The Environment contains everything that contributes to the specific
 */

ENV_API UUID create_environment();

/*
 * Adding an Image Based Lighting skybox to the scene.
 * Most image formats are supported, though you should use HDR images.
 */
ENV_API bool add_ibl_skybox(UUID env_id, const char* file_path);

ENV_API bool add_lit_material(UUID env_id, const char* material_name,
                                 float3 base_color = { 1.00f, 1.00f, 1.00f },
                                 float roughness = 0.00f,
                                 float metallic = 0.00f,
                                 float reflectance = 0.00f,
                                 float sheen_color = 0.00f,
                                 float clear_coat = 0.00f,
                                 float clear_coat_roughness = 0.00f);

ENV_API bool add_unlit_material(UUID env_id, const char* material_name,
                                   float3 base_color = {1.00f, 1.00f, 1.00f},
                                   float4 emmisive = {0.00f, 0.00f, 0.00f, 0.00f});

// Importing .gltf or .glb files.
// gltf animations are not played automatically!
ENV_API UUID add_gltf_asset_and_create_instance(UUID env_id, const char* filepath);

// Create another instance using the parent gltf asset from this instance.
ENV_API UUID create_gltf_instance_sibling(UUID gltf_instance_id);

// Importing .filamesh mesh files.
ENV_API UUID add_filamesh_from_file(UUID env_id, const char* path);

// Adding basic objects.
ENV_API UUID add_plane(UUID env_id, double3 center, double length_x, double length_z, const char* material_name, Quaternion rotation = identity_quaternion());
ENV_API UUID add_line(UUID env_id, double3 begin, double3 end, const char* material_name);

// Constructs lines in between neighbouring points.
// /*TODO*/ ENV_API UUID add_path(UUID env_id, double3* points, uint32_t n_points, const char* material);
// /*TODO*/ ENV_API UUID add_spere(UUID env_id, double3 center, double radius, const char* material);
// /*TODO*/ ENV_API UUID add_light(UUID env_id, double3 center, const char* material);

/* 
 * Frame Handling
 *
 * The Frame is what the image gets rendered into. It is mainly a wrapper around Filaments SwapChain.
 */

ENV_API UUID create_frame(UUID env_id, uint32_t width, uint32_t height);
ENV_API bool enable_pixel_capture(UUID frame_id, filament::backend::PixelDataFormat pixel_data_format, filament::backend::PixelDataType pixel_data_type);
ENV_API bool get_pixel_data(UUID frame_id, void** pixel_data, uint32_t* width, uint32_t* height);
ENV_API bool disable_pixel_capture(UUID frame_id);

/* 
 * Camera Handling
 */

ENV_API UUID create_camera(UUID env_id,
                              double3 pos = { 0.0, 0.0, 5.0 },
                              double3 lookat = { 0.0, 0.0, 0.0 },
                              double3 up = { 0.0, 1.0, 0.0 },
                              double vertical_fov = 60.0,
                              double near_plane = 0.1,
                              double far_plane = 50.0,
                              uint32_t width = 800,
                              uint32_t height = 600);

ENV_API bool set_camera_fov_vertical(UUID camera_id, double vertical_fov);
ENV_API double get_camera_fov_vertical(UUID camera_id);
ENV_API double3 get_camera_up_vector(UUID camera_id);
ENV_API double3 get_camera_forward_vector(UUID camera_id);
ENV_API bool render_frame(UUID camera_id, UUID frame_id);

/*
 * Window Handling
 */

ENV_API UUID create_window(UUID camera_id, int target_fps, const char* name);
ENV_API bool window_should_close(UUID window_id);
ENV_API bool update_window(UUID window_id); // only call, when 'window_should_close()' returns false
ENV_API double get_last_frame_time_ms(UUID window_id);

/*
 * Window User Input
 */


// Keyboard

ENV_API bool is_key_down(UUID window_id, SDL_Scancode code);
ENV_API bool is_key_pressed(UUID window_id, SDL_Scancode code);
ENV_API bool is_key_up(UUID window_id, SDL_Scancode code);
ENV_API bool is_key_released(UUID window_id, SDL_Scancode code);

// Mouse

enum Mouse_Button {
    MOUSE_BUTTON_NONE = 0,
    MOUSE_BUTTON_LEFT = SDL_BUTTON_LEFT,
    MOUSE_BUTTON_MIDDLE = SDL_BUTTON_MIDDLE,
    MOUSE_BUTTON_RIGHT = SDL_BUTTON_RIGHT
};

ENV_API bool is_mouse_button_down(UUID window_id, Mouse_Button button);
ENV_API bool is_mouse_button_pressed(UUID window_id, Mouse_Button button);
ENV_API bool is_mouse_button_up(UUID window_id, Mouse_Button button);
ENV_API bool is_mouse_button_released(UUID window_id, Mouse_Button button);
ENV_API int2 get_mouse_pos(UUID window_id);
ENV_API int2 get_mouse_delta(UUID window_id);
ENV_API int2 get_mouse_wheel_delta(UUID window_id);

/*
 * Entity Transformation
 */

ENV_API double3 get_position(UUID obj_id);

ENV_API bool set_position(UUID obj_id, double3 pos);
ENV_API bool set_orientation(UUID obj_id, Quaternion orientation);
ENV_API bool set_position_and_orientation(UUID obj_id, double3 pos, Quaternion orientation);

}
