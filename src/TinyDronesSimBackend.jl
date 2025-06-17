
const TDS_INVALID_ID = 0
@kwdef struct Environment_ID id::UInt64 = TDS_INVALID_ID end
@kwdef struct Frame_ID id::UInt64 = TDS_INVALID_ID end
@kwdef struct Camera_ID id::UInt64 = TDS_INVALID_ID end
@kwdef struct Viewport_ID id::UInt64 = TDS_INVALID_ID end
@kwdef struct Entity_ID id::UInt64 = TDS_INVALID_ID end
@kwdef struct glTF_Instance_ID id::UInt64 = TDS_INVALID_ID end
@kwdef struct Material_Instance_ID id::UInt64 = TDS_INVALID_ID end
@kwdef struct Joystick_ID id::UInt64 = TDS_INVALID_ID end

invalid_id(id) = id.id == TDS_INVALID_ID

const Int32_2 = SVector{2, Int32}
const UInt32_2 = SVector{2, UInt32}
const Float32_3 = SVector{3, Float32}
const Float32_4 = SVector{4, Float32}
const Float64_3 = SVector{3, Float64}
const Quaternion = SVector{4, Float64} # q = w + x * i + y * j + z * k

#####################
# Master controll
#####################

libtds = "./TinyDronesSimBackend/build/lib/libtds.so"

set_libtds_path(path) = global libtds = path
destroy_everything()::Bool = @ccall libtds.destroy_everything()::Bool
gui_continue()::Int32 = @ccall libtds.gui_continue()::Cint

#####################
# Environment
#####################    

env_create()::Environment_ID = @ccall libtds.env_create()::Environment_ID
env_exists(env::Environment_ID)::Bool = @ccall libtds.env_exists(env::Environment_ID)::Bool
env_destroy(env::Environment_ID)::Bool = @ccall libtds.env_destroy(env::Environment_ID)::Bool
"Change the 'active environment' to different environment."
env_activate(env::Environment_ID)::Bool = @ccall libtds.env_activate(env::Environment_ID)::Bool

"Adding an Image-Based-Lighting skybox to the scene. Currently only .hdr files are supported."
env_add_ibl_skybox(file_path::String)::Bool =  @ccall libtds.env_add_ibl_skybox(file_path::Cstring)::Bool

function env_add_lit_material(material_name::String = "DefaultMaterial";
                              base_color = Float64_3(1.0, 1.0, 1.0),
                              roughness = 0.0,
                              metallic = 0.0,
                              reflectance = 0.0,
                              sheen_color = 0.0,
                              clear_coat = 0.0,
                              clear_coat_roughness = 0.0)
    
    @ccall libtds.env_add_lit_material(material_name::Cstring,
                                       base_color::Float32_3,
                                       roughness::Float32,
                                       metallic::Float32,
                                       reflectance::Float32,
                                       sheen_color::Float32,
                                       clear_coat::Float32,
                                       clear_coat_roughness::Float32)::Bool
end

function env_add_unlit_material(material_name::String = "DefaultMaterial";
                            base_color = Float64_3(1.0, 1.0, 1.0),
                            emmisive = Float64_4(0.0, 0.0, 0.0, 0.0))::Bool

    @ccall libtds.env_add_unlit_material(material_name::Cstring,
                                         base_color::Float32_3,
                                         emmisive::Float32_4)::Bool
end

#####################
# glTF Object
#####################

"Importing .gltf or .glb files"
env_add_gltf_asset_and_create_instance(filepath::String)::glTF_Instance_ID = @ccall libtds.env_add_gltf_asset_and_create_instance(filepath::Cstring)::glTF_Instance_ID
"Create another instance using the parent gltf asset from this instance."
gltf_inst_create_sibling(gltf_inst_id::glTF_Instance_ID)::glTF_Instance_ID = @ccall libtds.gltf_inst_create_sibling(gltf_inst_id::glTF_Instance_ID)::glTF_Instance_ID
gltf_inst_position(gltf_inst_id::glTF_Instance_ID)::Float64_3 = @ccall libtds.gltf_inst_position(gltf_inst_id::glTF_Instance_ID)::Float64_3
gltf_inst_set_position(gltf_inst_id::glTF_Instance_ID, pos) = @ccall libtds.gltf_inst_set_position(gltf_inst_id::glTF_Instance_ID, pos::Float64_3)::Bool
gltf_inst_set_orientation(gltf_inst_id::glTF_Instance_ID, orientation::Quaternion)::Bool = @ccall libtds.gltf_inst_set_orientation(gltf_inst_id::glTF_Instance_ID, orientation::Quaternion)::Bool
gltf_inst_set_position_and_orientation(gltf_inst_id::glTF_Instance_ID, pos, orientation::Quaternion)::Bool = @ccall libtds.gltf_inst_set_position_and_orientation(gltf_inst_id::glTF_Instance_ID, pos::Float64_3, orientation::Quaternion)::Bool
gltf_inst_exists(gltf_inst_id::glTF_Instance_ID)::Bool = @ccall libtds.gltf_inst_exists(gltf_inst_id::glTF_Instance_ID)::Bool

#####################
# Filament Entity
#####################

"Importing .filamesh mesh files. They can be generated with a tool from Google-Filament"
env_add_filamesh_from_file(path::String)::Entity_ID = @ccall libtds.env_add_filamesh_from_file(path::Cstring)::Entity_ID

env_add_plane(center, length_x, length_z, material_name::String; rotation::Quaternion = identity_quaternion())::Entity_ID = @ccall libtds.env_add_plane(center::Float64_3, length_x::Float64, length_z::Float64, material_name::Cstring, rotation::Quaternion)::Entity_ID

env_add_line(begin_point, end_point, material_name::String)::Entity_ID = @ccall libtds.env_add_line(begin_point::Float64_3, end_point::Float64_3, material_name::Cstring)::Entity_ID

entity_position(entity::Entity_ID)::Float64_3 = @ccall libtds.entity_position(entity::Entity_ID)::Float64_3
entity_set_position(entity::Entity_ID, pos)::Bool = @ccall libtds.entity_set_position(entity::Entity_ID, pos::Float64_3)::Bool
entity_set_orientation(entity::Entity_ID, orientation::Quaternion)::Bool = @ccall libtds.entity_set_orientation(entity::Entity_ID, orientation::Quaternion)::Bool
entity_set_position_and_orientation(entity::Entity_ID, pos, orientation::Quaternion)::Bool = @ccall libtds.entity_set_position_and_orientation(entity::Entity_ID, pos::Float64_3, orientation::Quaternion)::Bool
entity_exists(entity::Entity_ID)::Bool = @ccall libtds.entity_exists(entity::Entity_ID)::Bool

#####################
# Frame
#####################

frame_create(env::Environment_ID, camera::Camera_ID, width, height)::Frame_ID = @ccall libtds.create_frame(env::Environment_ID, camera::Camera_ID, width::UInt32, height::UInt32)::Frame_ID
frame_exists(frame::Frame_ID)::Bool = @ccall libtds.frame_exists(frame::Frame_ID)::Bool
frame_destroy(frame::Frame_ID)::Bool = @ccall libtds.frame_destroy(frame::Frame_ID)::Bool

frame_size(frame::Frame_ID)::UInt32 = @ccall libtds.frame_size(frame::Frame_ID)::UInt32_2
frame_set_size(frame::Frame_ID, width::UInt32, height::UInt32)::Bool = @ccall libtds.frame_set_size(frame::Frame_ID, width::UInt32, height::UInt32)::Bool

# frame_enable_pixel_capture(frame::Frame_ID)::Bool = @ccall libtds.frame_enable_pixel_capture(frame::Frame_ID, )::Bool
frame_get_pixel_data(frame::Frame_ID, pixel_data::Ptr{Ptr{Cvoid}})::Bool = @ccall libtds.get_pixel_data(frame::Frame_ID, pixel_data::Ptr{Ptr{Cvoid}})::Bool
frame_disable_pixel_capture(frame::Frame_ID)::Bool = @ccall libtds.frame_disable_pixel_capture(frame::Frame_ID)::Bool

#####################
# Camera
#####################

function camera_create(env::Environment_ID;
                       pos = Float64_3(0.0, 0.0, 5.0),
                       lookat = Float64_3(0.0, 0.0, 0.0),
                       up = Float64_3(0.0, 1.0, 0.0),
                       vertical_fov = 60.0,
                       aspect_ratio = 16.0 / 9.0,
                       near_plane = 0.1,
                       far_plane = 50.0)::Camera_ID

    @ccall libtds.camera_create(env::Environment_ID,
                                pos::Float64_3,
                                lookat::Float64_3,
                                up::Float64_3,
                                vertical_fov::Float64,
                                aspect_ratio::Float64,
                                near_plane::Float64,
                                far_plane::Float64)::Camera_ID
end

camera_exists(camera::Camera_ID)::Bool = @ccall libtds.camera_exists(camera::Camera_ID)::Bool
camera_destroy(camera::Camera_ID)::Bool = @ccall libtds.camera_destroy(camera::Camera_ID)::Bool

camera_set_position(camera::Camera_ID, pos)::Bool = @ccall libtds.camera_set_position(camera::Camera_ID, pos::Float64_3)::Bool
camera_set_orientation(camera::Camera_ID, orientation::Quaternion)::Bool = @ccall libtds.camera_set_orientation(camera::Camera_ID, orientation::Quaternion)::Bool
camera_set_position_and_orientation(camera::Camera_ID, pos, orientation::Quaternion)::Bool = @ccall libtds.camera_set_position_and_orientation(camera::Camera_ID, pos::Float64_3, orientation::Quaternion)::Bool

camera_set_fov_vertical(camera::Camera_ID, vertical_fov)::Bool = @ccall libtds.set_camera_fov_vertical(camera::Camera_ID, vertical_fov::Float64)::Bool
camera_fov_vertical(camera::Camera_ID)::Float64 = @ccall libtds.camera_fov_vertical(camera::Camera_ID)::Float64

camera_up_vector(camera::Camera_ID)::Float64_3 = @ccall libtds.camera_up_vector(camera::Camera_ID)::Float64_3
camera_forward_vector(camera::Camera_ID)::Float64_3 = @ccall libtds.camera_forward_vector(camera::Camera_ID)::Float64_3

#####################
# Viewport
#####################

viewport_create()::Viewport_ID = @ccall libtds.viewport_create()::Viewport_ID
viewport_exists(viewport::Viewport_ID)::Bool = @ccall libtds.viewport_exists(viewport::Viewport_ID)::Bool
viewport_destroy(viewport::Viewport_ID)::Bool = @ccall libtds.viewport_destroy(viewport::Viewport_ID)::Bool

struct Viewport_Camera_Callback_Update
    viewport::Viewport_ID
    camera::Camera_ID
    env::Environment_ID
    
    user_ptr::Ptr{Cvoid}
    
    delta_time_us::UInt64
    delta_mouse_wheel::Int32
    delta_mouse_pos::Int32_2
    ui_hovered::Bool
end

@enum Viewport_Frame_Flags begin
    VIEWPORT_AUTOMATIC_RENDERING = 1 << 0
end

"""
Add a frame to a viewport. The viewport will not render the frame automatically unless specified with the flag 'VIEWPORT_AUTOMATIC_RENDERING'.
If possible add a camera instead of a frame, because the frame will be rendered from cpu-memory.
"""
function viewport_add_frame(viewport::Viewport_ID, frame::Frame_ID, flags::Viewport_Frame_Flags, tab_name::String, user_callback; user_ptr = nothing)::Bool
    user_callback_c = Ptr{Cvoid}(0)
    if user_callback != nothing
        user_callback_c = @cfunction($user_callback, UInt32, (Viewport_Camera_Callback_Update, ))
    end
    user_ptr_c = Ptr{Cvoid}(0)
    if user_ptr != nothing
        user_ptr_c = user_ptr
    end
    @ccall libtds.viewport_add_frame(viewport::Viewport_ID, frame::Frame_ID, flags::Viewport_Frame_Flags, tab_name::Cstring, user_callback_c::Ptr{Cvoid}, user_ptr_c::Ptr{Cvoid})::Bool
end

"Add a camera to a viewport. The viewport will internally create a frame and automatically render the view of the camera."
function viewport_add_camera(viewport::Viewport_ID, camera::Camera_ID, tab_name::String, user_callback; user_ptr = nothing)::Bool
    user_callback_c = Ptr{Cvoid}(0)
    if user_callback != nothing
        user_callback_c = @cfunction($user_callback, UInt32, (Viewport_Camera_Callback_Update, ))
    end
    user_ptr_c = Ptr{Cvoid}(0)
    if user_ptr != nothing
        user_ptr_c = user_ptr
    end
    @ccall libtds.viewport_add_camera(viewport::Viewport_ID, camera::Camera_ID, tab_name::Cstring, user_callback_c::Ptr{Cvoid}, user_ptr_c::Ptr{Cvoid})::Bool
end

"Change the 'active viewport' to different viewport."
viewport_activate(viewport::Viewport_ID)::Bool = @ccall libtds.viewport_activate(viewport::Viewport_ID)::Bool
is_active_viewport_set()::Bool = @ccall libtds.is_active_viewport_set()::Bool

viewport_key_down(ascii_code::Char)::Bool = @ccall libtds.viewport_key_down(ascii_code::UInt8)::Bool
viewport_mouse_pos()::Int32_2 = @ccall libtds.viewport_mouse_pos()::Int32_2
viewport_mouse_scroll_pos()::Int64 = @ccall libtds.viewport_mouse_scroll_pos()::Int64

@enum Mouse_Button begin
    MOUSE_BUTTON_LEFT = 1
    MOUSE_BUTTON_MIDDLE = 2
    MOUSE_BUTTON_RIGHT = 3
end

viewport_mouse_button_down(button::Mouse_Button)::Bool = @ccall libtds.viewport_mouse_button_down(button::UInt32)::Bool

#####################
# Joystick
#####################

const MAX_JOYSTICK_LENGTH = 16

struct Joystick_Raw
    axes_length::UInt64
    axes::SVector{MAX_JOYSTICK_LENGTH, Int16}
    buttons_length::UInt64
    buttons::SVector{MAX_JOYSTICK_LENGTH, Bool}
end

@enum Joystick_Axis begin
    JOYSTICK_THROTTLE = 1
    JOYSTICK_YAW = 2
    JOYSTICK_PITCH = 3
    JOYSTICK_ROLL = 4
end

struct Joystick_Data
    throttle::Float64
    yaw::Float64
    pitch::Float64
    roll::Float64
end

joystick_create()::Joystick_ID = @ccall libtds.joystick_create()::Joystick_ID
joystick_create_and_connect(device::String)::Joystick_ID = @ccall libtds.joystick_create_and_connect(device::Cstring)::Joystick_ID
joystick_connect(joystick::Joystick_ID, device::String)::Bool = @ccall libtds.joystick_connect(joystick::Joystick_ID, device::Cstring)::Bool
joystick_disconnect(joystick::Joystick_ID)::Bool = @ccall libtds.joystick_disconnect(joystick::Joystick_ID)::Bool
joystick_is_connected(joystick::Joystick_ID)::Bool = @ccall libtds.joystick_is_connected(joystick::Joystick_ID)::Bool
joystick_exists(joystick::Joystick_ID)::Bool = @ccall libtds.joystick_exists(joystick::Joystick_ID)::Bool
joystick_destroy(joystick::Joystick_ID)::Bool = @ccall libtds.joystick_destroy(joystick::Joystick_ID)::Bool
joystick_raw(joystick::Joystick_ID)::Joystick_Raw = @ccall libtds.joystick_raw(joystick::Joystick_ID)::Joystick_Raw
joystick_map_axis_idx_to_type(joystick::Joystick_ID, axis_idx::UInt8, axis_type::Joystick_Axis)::Bool = @ccall libtds.joystick_map_axis_idx_to_type(joystick::Joystick_ID, (axis_idx - 1)::UInt8, axis_type::Joystick_Axis)::Bool # axis_idx - 1 to correct for julia 1 based indexing
joystick_set_axis_range(joystick::Joystick_ID, axis_type::Joystick_Axis, min, center, max)::Bool = @ccall libtds.joystick_set_axis_range(joystick::Joystick_ID, axis_type::Joystick_Axis, min::Int16, center::Int16, max::Int16)::Bool
joystick_invert_axis(joystick::Joystick_ID, axis_type::Joystick_Axis)::Bool = @ccall libtds.joystick_invert_axis(joystick::Joystick_ID, axis_type::Joystick_Axis)::Bool
joystick_data(joystick::Joystick_ID)::Joystick_Data = @ccall libtds.joystick_data(joystick::Joystick_ID)::Joystick_Data
