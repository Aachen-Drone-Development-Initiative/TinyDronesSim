#pragma once

#include <cstddef>
#include <cstdint>

// Universal Unique Identifier
struct UUID {
    uint64_t id = 0;
    bool operator==(const UUID& other) const { return id == other.id; }
    bool operator!=(const UUID& other) const { return id != other.id; }
};

#define TDS_INVALID_ID UUID{0}

struct Environment_ID : public UUID {};
struct Frame_ID : public UUID {};
struct Camera_ID : public UUID {};
struct Viewport_ID : public UUID {};
struct Entity_ID : public UUID {};
struct glTF_Instance_ID : public UUID {};
struct Joystick_ID : public UUID {};

struct int2 { int x, y; };
struct uint32_2 { uint32_t x, y; };
struct float3 { float x, y, z; };
struct float4 { float x, y, z, w; };
struct double3 { double x, y, z; };
struct Quaternion { double x, y, z, w; };

#define TDS_API extern "C" __attribute__((visibility("default")))

/////////////////////
// Master controll
/////////////////////

TDS_API bool destroy_everything();
TDS_API int gui_continue();

/////////////////////
// Environment
/////////////////////

TDS_API Environment_ID env_create(); // automatically activates the environment
TDS_API bool env_exists(Environment_ID);
TDS_API bool env_destroy(Environment_ID);
TDS_API bool env_activate(Environment_ID);

/*
 * Add an Image Based Lighting skybox to the scene.
 * Most image formats are supported, though you should use HDR images.
 */
TDS_API bool env_add_ibl_skybox(const char* file_path);

enum Material_Parameter_Types : uint8_t {
    MAT_FLOAT32   = 1,
    MAT_FLOAT32_3 = 2,
    MAT_FLOAT32_4 = 3
};

TDS_API bool env_add_lit_material(const char* material_name,
                                  float3 base_color = { 1.00f, 1.00f, 1.00f },
                                  float roughness = 0.00f,
                                  float metallic = 0.00f,
                                  float reflectance = 0.00f,
                                  float sheen_color = 0.00f,
                                  float clear_coat = 0.00f,
                                  float clear_coat_roughness = 0.00f);

TDS_API bool env_add_unlit_material(const char* material_name,
                                    float3 base_color = {1.00f, 1.00f, 1.00f},
                                    float4 emmisive = {0.00f, 0.00f, 0.00f, 0.00f});

/////////////////////
// glTF Object
/////////////////////

TDS_API glTF_Instance_ID env_add_gltf_asset_and_create_instance(const char* filepath); // Importing .gltf or .glb files.
TDS_API glTF_Instance_ID gltf_inst_create_sibling(glTF_Instance_ID); // Create another instance using the parent gltf asset from this instance.
TDS_API double3 gltf_inst_position(glTF_Instance_ID);
TDS_API bool gltf_inst_set_position(glTF_Instance_ID, double3 pos);
TDS_API bool gltf_inst_set_orientation(glTF_Instance_ID, Quaternion orientation);
TDS_API bool gltf_inst_set_position_and_orientation(glTF_Instance_ID, double3 pos, Quaternion orientation);
TDS_API bool gltf_inst_exists(glTF_Instance_ID);

/////////////////////
// Filament Entity
/////////////////////

TDS_API Entity_ID env_add_filamesh_from_file(const char* path); // Importing .filamesh mesh files.
TDS_API Entity_ID env_add_plane(double3 center, double length_x, double length_z, const char* material_name, Quaternion rotation);
TDS_API Entity_ID env_add_line(double3 begin, double3 end, const char* material_name);

TDS_API double3 entity_position(Entity_ID);
TDS_API bool entity_set_position(Entity_ID, double3 pos);
TDS_API bool entity_set_orientation(Entity_ID, Quaternion orientation);
TDS_API bool entity_set_position_and_orientation(Entity_ID, double3 pos, Quaternion orientation);
TDS_API bool entity_exists(Entity_ID);

/////////////////////
// Frame
/////////////////////

TDS_API Frame_ID frame_create(Environment_ID, Camera_ID, uint32_t width, uint32_t height);
TDS_API bool frame_exists(Frame_ID);
TDS_API bool frame_destroy(Frame_ID);

TDS_API uint32_2 frame_size(Frame_ID);
TDS_API bool frame_set_size(Frame_ID, uint32_t width, uint32_t height);

// TDS_API bool frame_enable_pixel_capture(Frame_ID, filament::backend::PixelDataFormat pixel_data_format, filament::backend::PixelDataType pixel_data_type);
// TDS_API uint32_2 frame_get_pixel_data(Frame_ID, void** pixel_data);
TDS_API bool frame_disable_pixel_capture(Frame_ID);

/////////////////////
// Camera
/////////////////////

TDS_API Camera_ID camera_create(Environment_ID,
                                double3 pos = { 0.0, 0.0, 5.0 },
                                double3 lookat = { 0.0, 0.0, 0.0 },
                                double3 up = { 0.0, 1.0, 0.0 },
                                double vertical_fov = 60.0,
                                double aspect_ratio = 16.0 / 9.0,
                                double near_plane = 0.1,
                                double far_plane = 50.0);

TDS_API bool camera_exists(Camera_ID);
TDS_API bool camera_destroy(Camera_ID);

TDS_API bool camera_set_position(Camera_ID, double3 pos);
TDS_API bool camera_set_orientation(Camera_ID, Quaternion orientation);
TDS_API bool camera_set_position_and_orientation(Camera_ID, double3 pos, Quaternion orientation);
TDS_API bool camera_set_fov_vertical(Camera_ID, double vertical_fov);
TDS_API double camera_fov_vertical(Camera_ID);
TDS_API double3 camera_up_vector(Camera_ID);
TDS_API double3 camera_forward_vector(Camera_ID);

/////////////////////
// Viewport
/////////////////////

TDS_API Viewport_ID viewport_create();
TDS_API bool viewport_exists(Viewport_ID);
TDS_API bool viewport_destroy(Viewport_ID);

struct Viewport_Camera_Callback_Update {
    Viewport_ID viewport_id;
    Camera_ID camera_id;
    Environment_ID env_id;

    void* user_ptr;
    
    uint64_t delta_time_us;
    int delta_mouse_wheel;
    int2 delta_mouse_pos;
    bool ui_hovered;
};

enum Viewport_Location : uint32_t {
    VIEWPORT_CENTER = 0,
    // VIEWPORT_LEFT = 1,
    VIEWPORT_RIGHT = 2,
    // VIEWPORT_UPPER = 3,
    VIEWPORT_DOWN = 4
};

enum Viewport_Frame_Flags : uint32_t {
    VIEWPORT_AUTOMATIC_RENDERING = 1 << 0,
};

TDS_API bool viewport_add_frame(Viewport_ID,
                                Frame_ID,
                                Viewport_Frame_Flags,
                                uint32_t (*user_callback)(Viewport_Camera_Callback_Update),
                                void* user_ptr,
                                const char* location_path,
                                const char* location_name);

TDS_API bool viewport_add_camera(Viewport_ID,
                                 Camera_ID,
                                 uint32_t (*user_callback)(Viewport_Camera_Callback_Update),
                                 void* user_ptr,
                                 const char* location_path,
                                 const char* location_name);

TDS_API bool viewport_add_controll_panel(Viewport_ID, bool vertical, const char* location_path, const char* location_name);

typedef void (*Viewport_Button_Callback)(void* user_ptr);
TDS_API bool viewport_controll_add_button(Viewport_ID, const char* label, Viewport_Button_Callback, void* user_ptr);

typedef void (*Viewport_Slider_Callback)(double, void* user_ptr);
TDS_API bool viewport_controll_add_slider(Viewport_ID, const char* label, Viewport_Slider_Callback, void* user_ptr);

TDS_API bool viewport_controll_add_label(Viewport_ID, const char* label);

TDS_API bool viewport_activate(Viewport_ID window_id); // Called automatically when a viewport is created
TDS_API bool is_active_viewport_set();

TDS_API bool viewport_key_down(uint8_t ascii_code);
TDS_API int2 viewport_mouse_pos();
TDS_API int64_t viewport_mouse_scroll_pos();

enum Mouse_Button : uint32_t {
    MOUSE_BUTTON_LEFT = 1,
    MOUSE_BUTTON_MIDDLE = 2,
    MOUSE_BUTTON_RIGHT = 3
};

TDS_API bool viewport_mouse_button_down(Mouse_Button);

/////////////////////
// Joystick
/////////////////////

constexpr size_t MAX_JOYSTICK_LENGTH = 32;

struct Joystick_Raw {
    size_t axes_length = 0;
    int16_t axes[MAX_JOYSTICK_LENGTH] = {};
    size_t buttons_length = 0;
    bool buttons[MAX_JOYSTICK_LENGTH] = {};
};

enum Joystick_Axis : uint32_t {
    JOYSTICK_THROTTLE = 1,
    JOYSTICK_YAW = 2,
    JOYSTICK_PITCH = 3,
    JOYSTICK_ROLL = 4
};

struct Joystick_Data {
    double throttle, yaw, pitch, roll;
};

TDS_API Joystick_ID joystick_create();
TDS_API Joystick_ID joystick_create_and_connect(const char* device);
TDS_API bool joystick_connect(Joystick_ID, const char* device);
TDS_API bool joystick_disconnect(Joystick_ID);
TDS_API bool joystick_is_connected(Joystick_ID);
TDS_API bool joystick_exists(Joystick_ID);
TDS_API bool joystick_destroy(Joystick_ID);
TDS_API Joystick_Raw joystick_raw(Joystick_ID);
TDS_API bool joystick_map_axis_idx_to_type(Joystick_ID, uint8_t axis_idx, Joystick_Axis axis_type);
TDS_API bool joystick_set_axis_range(Joystick_ID, Joystick_Axis axis_type, int16_t min, int16_t center, int16_t max);
TDS_API bool joystick_invert_axis(Joystick_ID, Joystick_Axis axis_type);
TDS_API Joystick_Data joystick_data(Joystick_ID);
