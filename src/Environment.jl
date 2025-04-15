using TinyDronesSim
using StaticStrings

const libenv = "EnvironmentBackend/build/lib/libenvironment.so"

export UUID

const UUID = UInt64

function add_renderables(env_id::UUID, obj::Nothing)::Nothing end
function update_renderables(obj::Nothing)::Nothing end

#
# State Handling
#

destroy(obj_id::UUID)::Bool = @ccall libenv.destroy(obj_id::UUID)::UInt8
destroy_everything()::Bool = @ccall libenv.destroy_everything()::UInt8

#
# Environment Handling
#
# The Environment contains everything that contributes to the specific
#

create_environment()::UUID = @ccall libenv.create_environment()::UUID

#
# Adding an Image Based Lighting skybox to the scene.
# Most image formats are supported, though you should use HDR images.
#
function add_ibl_skybox(env_id::UUID, file_path::CStaticString{N})::Bool where N
    @ccall libenv.add_ibl_skybox(env_id::UUID, file_path::Cstring)::UInt8
end

function add_lit_material(env_id::UUID, material_name::CStaticString{N} = cstatic"DefaultMaterial";
                          base_color::Float64_3 = Float64_3(1.0, 1.0, 1.0),
                          roughness::Float64 = 0.0,
                          metallic::Float64 = 0.0,
                          reflectance::Float64 = 0.0,
                          sheen_color::Float64 = 0.0,
                          clear_coat::Float64 = 0.0,
                          clear_coat_roughness::Float64 = 0.0)::Bool where N
    
    @ccall libenv.add_lit_material(env_id::UUID,
                                   material_name::Cstring,
                                   base_color::Float32_3,
                                   roughness::Float32,
                                   metallic::Float32,
                                   reflectance::Float32,
                                   sheen_color::Float32,
                                   clear_coat::Float32,
                                   clear_coat_roughness::Float32)::UInt8
end

function add_unlit_material(env_id::UUID, material_name::CStaticString{N} = cstatic"DefaultMaterial";
                            base_color::Float64_3 = Float64_3(1.0, 1.0, 1.0),
                            emmisive::Float64_4 = Float64_4(0.0, 0.0, 0.0, 0.0))::Bool where N

    @ccall libenv.add_unlit_material(env_id::UUID,
                                     material_name::Cstring,
                                     base_color::Float32_3,
                                     emmisive::Float32_4)::UInt8
end

# Importing .gltf or .glb files.
# gltf animations are not played automatically!
function add_gltf_asset_and_create_instance(env_id::UUID, filepath::CStaticString{N})::UUID where N
    @ccall libenv.add_gltf_asset_and_create_instance(env_id::UUID, filepath::Cstring)::UUID
end

# Create another instance using the parent gltf asset from this instance.
create_gltf_instance_sibling(gltf_instance_id::UUID)::UUID = @ccall libenv.create_gltf_instance_sibling(gltf_instance_id::UUID)::UUID

# Importing .filamesh mesh files.
function add_filamesh_from_file(env_id::UUID, path::CStaticString{N})::UUID where N
    @ccall libenv.add_filamesh_from_file(env_id::UUID, path::Cstring)::UUID
end

# Adding basic objects.
function add_plane(env_id::UUID, center::Float64_3, length_x::Float64, length_z::Float64, material_name::CStaticString{N}, rotation::Quaternion = identity_quaternion())::UUID where N
    @ccall libenv.add_plane(env_id::UUID, center::Float64_3, length_x::Float64, length_z::Float64, material_name::Cstring, rotation::Quaternion)::UUID
end

function add_line(env_id::UUID, begin_point::Float64_3, end_point::Float64_3, material_name::CStaticString{N})::UUID where N
    @ccall libenv.add_line(env_id::UUID, begin_point::Float64_3, end_point::Float64_3, material_name::Cstring)::UUID
end
    
# 
# Frame Handling
#
# The Frame is what the image gets rendered into. It is mainly a wrapper around Filaments SwapChain.
#

create_frame(env_id::UUID, width::UInt32, height::UInt32)::UUID = @ccall libenv.create_frame(env_id::UUID, width::UInt32, height::UInt32)::UUID
get_pixel_data(frame_id::UUID, pixel_data::Ptr{Ptr{Cvoid}}, width::Ptr{UInt32}, height::Ptr{UInt32})::Bool = @ccall libenv.get_pixel_data(frame_id::UUID, pixel_data::Ptr{Ptr{Cvoid}}, width::Ptr{UInt32}, height::Ptr{UInt32})::UInt8
disable_pixel_capture(frame_id::UUID)::Bool = @ccall libenv.disable_pixel_capture(frame_id::UUID)::UInt8

# 
# Camera Handling
#

function create_camera(env_id::UUID;
                       pos::Float64_3 = Float64_3(0.0, 0.0, 5.0),
                       lookat::Float64_3 = Float64_3(0.0, 0.0, 0.0),
                       up::Float64_3 = Float64_3(0.0, 1.0, 0.0),
                       vertical_fov::Float64 = 60.0,
                       near_plane::Float64 = 0.1,
                       far_plane::Float64 = 50.0,
                       width::UInt32 = UInt32(800),
                       height::UInt32 = UInt32(600))::UUID

    @ccall libenv.create_camera(env_id::UUID,
                                pos::Float64_3,
                                lookat::Float64_3,
                                up::Float64_3,
                                vertical_fov::Float64,
                                near_plane::Float64,
                                far_plane::Float64,
                                width::UInt32,
                                height::UInt32)::UUID
end

set_camera_fov_vertical(camera_id::UUID, vertical_fov::Float64)::Bool = @ccall libenv.set_camera_fov_vertical(camera_id::UUID, vertical_fov::Float64)::UInt8
get_camera_fov_vertical(camera_id::UUID)::Float64 = @ccall libenv.get_camera_fov_vertical(camera_id::UUID)::Float64
get_camera_up_vector(camera_id::UUID)::Float64_3 = @ccall libenv.get_camera_up_vector(camera_id::UUID)::Float64_3
get_camera_forward_vector(camera_id::UUID)::Float64_3 = @ccall libenv.get_camera_forward_vector(camera_id::UUID)::Float64_3
render_frame(camera_id::UUID, frame_id::UUID)::Bool = @ccall libenv.render_frame(camera_id::UUID, frame_id::UUID)::UInt8

#
# Window Handling
#

function create_window(camera_id::UUID, name::CStaticString{N}; target_fps::Int32 = 60)::UUID where N
    @ccall libenv.create_window(camera_id::UUID, target_fps::Int32, name::Cstring)::UUID
end

window_should_close(window_id::UUID)::Bool = @ccall libenv.window_should_close(window_id::UUID)::UInt8
update_window(window_id::UUID)::Bool = @ccall libenv.update_window(window_id::UUID)::UInt8 # only call, when 'window_should_close()' returns false
get_last_frame_time_ms(window_id::UUID)::Float64 = @ccall libenv.get_last_frame_time_ms(window_id::UUID)::Float64

#
# Window User Input
#

# Keyboard

@enum SDL_Scancode begin
    SDL_SCANCODE_A = 4
    SDL_SCANCODE_D = 7
    SDL_SCANCODE_S = 22
    SDL_SCANCODE_W = 26
    SDL_SCANCODE_SPACE = 44
end

is_key_down(window_id::UUID, code::SDL_Scancode)::Bool = @ccall libenv.is_key_down(window_id::UUID, code::SDL_Scancode)::UInt8
is_key_pressed(window_id::UUID, code::SDL_Scancode)::Bool = @ccall libenv.is_key_pressed(window_id::UUID, code::SDL_Scancode)::UInt8
is_key_up(window_id::UUID, code::SDL_Scancode)::Bool = @ccall libenv.is_key_up(window_id::UUID, code::SDL_Scancode)::UInt8
is_key_released(window_id::UUID, code::SDL_Scancode)::Bool = @ccall libenv.is_key_released(window_id::UUID, code::SDL_Scancode)::UInt8

# Mouse

@enum Mouse_Button begin
    MOUSE_BUTTON_NONE = 0
    MOUSE_BUTTON_LEFT = 1
    MOUSE_BUTTON_MIDDLE = 2
    MOUSE_BUTTON_RIGHT = 3
end

is_mouse_button_down(window_id::UUID, button::Mouse_Button)::Bool = @ccall libenv.is_mouse_button_down(window_id::UUID, button::Mouse_Button)::UInt8
is_mouse_button_pressed(window_id::UUID, button::Mouse_Button)::Bool = @ccall libenv.is_mouse_button_pressed(window_id::UUID, button::Mouse_Button)::UInt8
is_mouse_button_up(window_id::UUID, button::Mouse_Button)::Bool = @ccall libenv.is_mouse_button_up(window_id::UUID, button::Mouse_Button)::UInt8
is_mouse_button_released(window_id::UUID, button::Mouse_Button)::Bool = @ccall libenv.is_mouse_button_released(window_id::UUID, button::Mouse_Button)::UInt8
get_mouse_pos(window_id::UUID)::Int32_2 = @ccall libenv.get_mouse_pos(window_id::UUID)::Int32_2
get_mouse_delta(window_id::UUID)::Int32_2 = @ccall libenv.get_mouse_delta(window_id::UUID)::Int32_2
get_mouse_wheel_delta(window_id::UUID)::Int32_2 = @ccall libenv.get_mouse_wheel_delta(window_id::UUID)::Int32_2

#
# Entity Transformation
#

get_position(obj_id::UUID)::Float64_3 = @ccall libenv.get_position(obj_id::UUID)::Float64_3

set_position(obj_id::UUID, pos::Float64_3)::Bool = @ccall libenv.set_position(obj_id::UUID, pos::Float64_3)::UInt8
set_orientation(obj_id::UUID, orientation::Quaternion)::Bool = @ccall libenv.set_orientation(obj_id::UUID, orientation::Quaternion)::UInt8
set_position_and_orientation(obj_id::UUID, pos::Float64_3, orientation::Quaternion)::Bool = @ccall libenv.set_position_and_orientation(obj_id::UUID, pos::Float64_3, orientation::Quaternion)::UInt8

#
# User Camera
#

@kwdef mutable struct Camera_Motion_State
    # we store the orientation and position to avoid repeated conversion from and to matrices
    orbit_center::Float64_3 = Float64_3(0.0, 0.0, 0.0)
    orientation::Quaternion = identity_quaternion()
    center_distance::Float64 = 1.0 # only used for THIRD_PERSON mode
end

@enum Camera_Motion_Mode begin
    FIRST_PERSON
    THIRD_PERSON
end

function update_camera_by_window_input_first_person!(camera::UUID, window::UUID, state::Camera_Motion_State,
                                                     zoom_sensitivity::Float64,
                                                     max_vertical_fov::Float64,
                                                     min_vertical_fov::Float64)
    # Zooming
    scroll_delta_effective = get_mouse_wheel_delta(window) .* zoom_sensitivity * get_last_frame_time_ms(window)
    prev_fov = get_camera_fov_vertical(camera)
    new_fov = prev_fov + prev_fov * scroll_delta_effective.y
    if (min_vertical_fov <= new_fov <= max_vertical_fov)
        set_camera_fov_vertical(camera, new_fov)
    end

    # Updating the camera
    set_position_and_orientation(camera, state.orbit_center, state.orientation)
end

function update_camera_by_window_input_third_person!(camera::UUID, window::UUID, state::Camera_Motion_State,
                                                     zoom_sensitivity::Float64,
                                                     max_center_distance::Float64, min_center_distance::Float64)
    # Zooming
    scroll_delta_effective = get_mouse_wheel_delta(window) .* zoom_sensitivity * get_last_frame_time_ms(window)
    new_center_distance = state.center_distance + state.center_distance * scroll_delta_effective.y
    if (min_center_distance <= new_center_distance <= max_center_distance)
        state.center_distance = new_center_distance
    end

    # Updating the camera
    set_orientation(camera, state.orientation)
    # The orientation needs to be updated first, so that the 'forward vector' we are "getting" is up to date,
    # otherwise there are laggy movement artifacts.
    set_position(camera, state.orbit_center - get_camera_forward_vector(camera) .* state.center_distance)
end

function update_camera_by_window_input!(camera::UUID, window::UUID, state::Camera_Motion_State;
                                                     mode::Camera_Motion_Mode = THIRD_PERSON,
                                                     rotation_sensitivity::Float64 = 0.001,
                                                     translation_sensitivity::Float64 = 0.003,
                                                     zoom_sensitivity::Float64 = 0.005,
                                                     walk_speed::Float64 = 0.5,
                                                     max_vertical_fov::Float64 = 100.0,
                                                     min_vertical_fov::Float64 = 5.0,
                                                     max_center_distance::Float64 = 10000.0,
                                                     min_center_distance::Float64 = 0.01)

    # Rotation
    if is_mouse_button_down(window, MOUSE_BUTTON_LEFT)
        mouse_delta_effective = -get_mouse_delta(window) .* rotation_sensitivity * get_last_frame_time_ms(window)
        
        orientation_delta_pitch = normed_axis_angle_to_quaternion(mouse_delta_effective.y, Float64_3(1.0, 0.0, 0.0)) # in local coordinate space
        orientation_delta_yaw = normed_axis_angle_to_quaternion(mouse_delta_effective.x, get_camera_up_vector(camera)) # in global coordinate space
        # The 'combine'-order is very important here.
        # First we pitch around the a vector pointing to the left of the camera in local coordinates (1,0,0),
        # then we rotate by the cameras previous orientation, and finally around the cameras 'up' vector.
        state.orientation = combine(combine(orientation_delta_pitch, state.orientation), orientation_delta_yaw)
    end
    
    # Walking
    if is_key_down(window, SDL_SCANCODE_W)
        state.orbit_center += get_camera_forward_vector(camera) .* (walk_speed / get_last_frame_time_ms(window))
    end
    if is_key_down(window, SDL_SCANCODE_S)
        state.orbit_center -= get_camera_forward_vector(camera) .* (walk_speed / get_last_frame_time_ms(window))
    end
    if is_key_down(window, SDL_SCANCODE_D)
        dir = norm_vector(cross_product(get_camera_forward_vector(camera), get_camera_up_vector(camera)))
        state.orbit_center += dir .* (walk_speed / get_last_frame_time_ms(window))
    end
    if is_key_down(window, SDL_SCANCODE_A)
        dir = norm_vector(cross_product(get_camera_forward_vector(camera), get_camera_up_vector(camera)))
        state.orbit_center -= dir .* (walk_speed / get_last_frame_time_ms(window))
    end

    # Paning
    if is_mouse_button_down(window, MOUSE_BUTTON_RIGHT)
        mouse_delta_effective = get_mouse_delta(window) .* translation_sensitivity
        forward = get_camera_forward_vector(camera)
        up = get_camera_up_vector(camera)
        right = norm_vector(cross_product(up, forward))
        head_dir = norm_vector(cross_product(forward, right))
        state.orbit_center += right .* mouse_delta_effective.x + head_dir .* mouse_delta_effective.y
    end
    
    if mode == FIRST_PERSON
        update_camera_by_window_input_first_person!(camera, window, state, zoom_sensitivity, max_vertical_fov, min_vertical_fov)
    elseif mode == THIRD_PERSON
        update_camera_by_window_input_third_person!(camera, window, state, zoom_sensitivity, max_center_distance, min_center_distance)
    end
end
