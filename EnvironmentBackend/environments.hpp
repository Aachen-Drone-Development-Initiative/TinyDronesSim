#pragma once

#include <math.hpp>

#include <cstdint>

#include <backend/DriverEnums.h>

#include <SDL_scancode.h>
#include <SDL_mouse.h>

#define ENV_API extern "C" __attribute__((visibility("default")))

// Universal Unique Identifier
struct UUID {
    uint64_t id = 0;
    bool operator==(const UUID& other) const { return id == other.id; }
};

#define ENV_INVALID_UUID UUID{0}

struct Environment_ID : public UUID {};
struct Frame_ID : public UUID {};
struct Camera_ID : public UUID {};
struct Window_ID : public UUID {};
struct Filament_Entity_ID : public UUID {};
struct glTF_Instance_ID : public UUID {};

/*
 * General Object Management
 */

ENV_API bool destroy_everything();

/*
 * Environment Handling
 *
 * The Environment contains everything that contributes to the specific
 */

ENV_API Environment_ID create_environment(); // automatically activates the environment
ENV_API bool environment_exists(Environment_ID env_id);
ENV_API bool destroy_environment(Environment_ID env_id);
ENV_API bool environment_activate(Environment_ID env_id);

/*
 * Adding an Image Based Lighting skybox to the scene.
 * Most image formats are supported, though you should use HDR images.
 */
ENV_API bool add_ibl_skybox(const char* file_path);

ENV_API bool add_lit_material(const char* material_name,
                              float3 base_color = { 1.00f, 1.00f, 1.00f },
                              float roughness = 0.00f,
                              float metallic = 0.00f,
                              float reflectance = 0.00f,
                              float sheen_color = 0.00f,
                              float clear_coat = 0.00f,
                              float clear_coat_roughness = 0.00f);

ENV_API bool add_unlit_material(const char* material_name,
                                float3 base_color = {1.00f, 1.00f, 1.00f},
                                float4 emmisive = {0.00f, 0.00f, 0.00f, 0.00f});
    
// Importing .gltf or .glb files.
// gltf animations are not played automatically!
ENV_API glTF_Instance_ID add_gltf_asset_and_create_instance(const char* filepath);

ENV_API Filament_Entity_ID get_gltf_instance_filament_entity(glTF_Instance_ID gltf_instance_id);

// Create another instance using the parent gltf asset from this instance.
ENV_API glTF_Instance_ID create_gltf_instance_sibling(glTF_Instance_ID gltf_instance_id);

// Importing .filamesh mesh files.
ENV_API Filament_Entity_ID add_filamesh_from_file(const char* path);

// Adding basic objects.
ENV_API Filament_Entity_ID add_plane(double3 center, double length_x, double length_z, const char* material_name, Quaternion rotation = identity_quaternion());
ENV_API Filament_Entity_ID add_line(double3 begin, double3 end, const char* material_name);

// Constructs lines in between neighbouring points.
// /*TODO*/ ENV_API UUID add_path(UUID env_id, double3* points, uint32_t n_points, const char* material);
// /*TODO*/ ENV_API UUID add_spere(UUID env_id, double3 center, double radius, const char* material);
// /*TODO*/ ENV_API UUID add_light(UUID env_id, double3 center, const char* material);

ENV_API bool filament_entity_exists(Filament_Entity_ID filament_entity_id);
ENV_API bool gltf_instance_exists(glTF_Instance_ID gltf_instance_id);

/* 
 * Frame Handling
 *
 * The Frame is what the image gets rendered into. It is mainly a wrapper around Filaments SwapChain.
 */

ENV_API Frame_ID create_frame(Environment_ID env_id, uint32_t width, uint32_t height);
ENV_API bool frame_exists(Frame_ID frame_id);
ENV_API bool destroy_frame(Frame_ID frame_id);

ENV_API bool enable_pixel_capture(Frame_ID frame_id, filament::backend::PixelDataFormat pixel_data_format, filament::backend::PixelDataType pixel_data_type);
ENV_API bool get_pixel_data(Frame_ID frame_id, void** pixel_data, uint32_t* width, uint32_t* height);
ENV_API bool disable_pixel_capture(Frame_ID frame_id);

/* 
 * Camera Handling
 */

ENV_API Camera_ID create_camera(Environment_ID env_id,
                                double3 pos = { 0.0, 0.0, 5.0 },
                                double3 lookat = { 0.0, 0.0, 0.0 },
                                double3 up = { 0.0, 1.0, 0.0 },
                                double vertical_fov = 60.0,
                                double near_plane = 0.1,
                                double far_plane = 50.0,
                                uint32_t width = 800,
                                uint32_t height = 600);
ENV_API bool camera_exists(Camera_ID camera_id);
ENV_API bool destroy_camera(Camera_ID camera_id);

ENV_API bool set_camera_fov_vertical(Camera_ID camera_id, double vertical_fov);
ENV_API double get_camera_fov_vertical(Camera_ID camera_id);
ENV_API double3 get_camera_up_vector(Camera_ID camera_id);
ENV_API double3 get_camera_forward_vector(Camera_ID camera_id);
ENV_API bool render_frame(Camera_ID camera_id, Frame_ID frame_id);

ENV_API Filament_Entity_ID get_camera_filament_entity(Camera_ID camera_id);

/*
 * Window Handling
 */

ENV_API Window_ID create_window(Camera_ID camera_id, int target_fps, const char* name);
ENV_API bool window_exists(Window_ID window_id);
ENV_API bool destroy_window(Window_ID window_id);

ENV_API bool window_visible(Window_ID window_id);
ENV_API bool window_show(Window_ID window_id);
ENV_API bool window_hide(Window_ID window_id);

ENV_API bool window_activate(Window_ID window_id);
ENV_API bool is_active_window_set();
ENV_API bool window_update();
ENV_API double window_get_last_frame_time_ms();
ENV_API bool window_give_input_focus(); // Focus input from keyboard, mouse, joystick, etc. to the active window.

/*
 * Window User Input
 */

#define ENV_MAX_MOUSE_BUTTONS 8
#define ENV_MAX_JOYSTICK_AXES 8

// Keyboard

ENV_API bool is_key_down(SDL_Scancode code);
ENV_API bool is_key_pressed(SDL_Scancode code);
ENV_API bool is_key_up(SDL_Scancode code);
ENV_API bool is_key_released(SDL_Scancode code);

// Mouse

enum Mouse_Button {
    MOUSE_BUTTON_NONE = 0,
    MOUSE_BUTTON_LEFT = SDL_BUTTON_LEFT,
    MOUSE_BUTTON_MIDDLE = SDL_BUTTON_MIDDLE,
    MOUSE_BUTTON_RIGHT = SDL_BUTTON_RIGHT
};

ENV_API bool is_mouse_button_down(Mouse_Button button);
ENV_API bool is_mouse_button_pressed(Mouse_Button button);
ENV_API bool is_mouse_button_up(Mouse_Button button);
ENV_API bool is_mouse_button_released(Mouse_Button button);
ENV_API int2 get_mouse_pos();
ENV_API int2 get_mouse_delta();
ENV_API int2 get_mouse_wheel_delta();

// Joystick

enum Joystick_Axis : uint8_t {
    JOYSTICK_NONE = 0,
    JOYSTICK_THROTTLE = 1,
    JOYSTICK_YAW = 2,
    JOYSTICK_PITCH = 3,
    JOYSTICK_ROLL = 4
};

ENV_API bool connect_to_joystick();
ENV_API bool is_connected_to_joystick();
ENV_API bool disconnect_from_joystick();
ENV_API int16_t get_joystick_axis_raw(uint8_t axis_idx);
ENV_API bool assign_joystick_axis_idx_to_axis_type(uint8_t axis_idx, Joystick_Axis axis_type);
ENV_API bool set_joystick_axis_range(Joystick_Axis axis_type, int16_t min, int16_t zero, int16_t max);
ENV_API double get_joystick_axis_mapped_value(Joystick_Axis axis_type);

/*
 * Entity Transformation
 */

ENV_API double3 get_position(Filament_Entity_ID filament_entity_id);

ENV_API bool set_position(Filament_Entity_ID filament_entity_id, double3 pos);
ENV_API bool set_orientation(Filament_Entity_ID filament_entity_id, Quaternion orientation);
ENV_API bool set_position_and_orientation(Filament_Entity_ID filament_entity_id, double3 pos, Quaternion orientation);
