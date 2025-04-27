using TinyDronesSim
using StaticStrings
using StaticArrays

const libenv = "EnvironmentBackend/build/lib/libenvironment.so"

const INVALID_UUID = 0

@kwdef struct Environment_ID id::UInt64 = INVALID_UUID end

@kwdef struct Frame_ID id::UInt64 = INVALID_UUID end
@kwdef struct Camera_ID id::UInt64 = INVALID_UUID end
@kwdef struct Window_ID id::UInt64 = INVALID_UUID end
@kwdef struct Filament_Entity_ID id::UInt64 = INVALID_UUID end
@kwdef struct glTF_Instance_ID id::UInt64 = INVALID_UUID end

#
# State Handling
#

destroy_everything()::Bool = @ccall libenv.destroy_everything()::Bool

#
# Environment Handling
#
# The Environment contains everything that contributes to the specific
#

create_environment()::Environment_ID = @ccall libenv.create_environment()::Environment_ID
exists(env::Environment_ID)::Bool = @ccall libenv.environment_exists(env::Environment_ID)::Bool
destroy(env::Environment_ID)::Bool = @ccall libenv.destroy_environment(env::Environment_ID)::Bool

"Change the 'active environment' to different environment."
set_active_environment(env::Environment_ID)::Bool = @ccall libenv.set_active_environment(env::Environment_ID)::Bool

#
# Adding an Image Based Lighting skybox to the scene.
# Most image formats are supported, though you should use HDR images.
#
function add_ibl_skybox(file_path::CStaticString{N})::Bool where N
    @ccall libenv.add_ibl_skybox(file_path::Cstring)::Bool
end

function add_lit_material(material_name::CStaticString{N} = cstatic"DefaultMaterial";
                          base_color = Float64_3(1.0, 1.0, 1.0),
                          roughness = 0.0,
                          metallic = 0.0,
                          reflectance = 0.0,
                          sheen_color = 0.0,
                          clear_coat = 0.0,
                          clear_coat_roughness = 0.0)::Bool where N
    
    @ccall libenv.add_lit_material(material_name::Cstring,
                                   base_color::Float32_3,
                                   roughness::Float32,
                                   metallic::Float32,
                                   reflectance::Float32,
                                   sheen_color::Float32,
                                   clear_coat::Float32,
                                   clear_coat_roughness::Float32)::Bool
end

function add_unlit_material(material_name::CStaticString{N} = cstatic"DefaultMaterial";
                            base_color = Float64_3(1.0, 1.0, 1.0),
                            emmisive = Float64_4(0.0, 0.0, 0.0, 0.0))::Bool where N

    @ccall libenv.add_unlit_material(material_name::Cstring,
                                     base_color::Float32_3,
                                     emmisive::Float32_4)::Bool
end

"Importing .gltf or .glb files. Animations are not played automatically!"
function add_gltf_asset_and_create_instance(filepath::CStaticString{N})::glTF_Instance_ID where N
    @ccall libenv.add_gltf_asset_and_create_instance(filepath::Cstring)::glTF_Instance_ID
end

"Get the associated filament entity, which can be used for changing pos/orientation among other things."
get_filament_entity(gltf_instance::glTF_Instance_ID)::Filament_Entity_ID = @ccall libenv.get_gltf_instance_filament_entity(gltf_instance::glTF_Instance_ID)::Filament_Entity_ID

"Create another instance using the parent gltf asset from this instance."
create_gltf_instance_sibling(gltf_instance::glTF_Instance_ID)::glTF_Instance_ID = @ccall libenv.create_gltf_instance_sibling(gltf_instance::glTF_Instance_ID)::glTF_Instance_ID

"Importing .filamesh mesh files. They can be generated with a tool from Google-Filament"
function add_filamesh_from_file(path::CStaticString{N})::Filament_Entity_ID where N
    @ccall libenv.add_filamesh_from_file(path::Cstring)::Filament_Entity_ID
end

# Adding basic objects.
function add_plane(center, length_x, length_z, material_name::CStaticString{N}; rotation::Quaternion = identity_quaternion())::Filament_Entity_ID where N
    @ccall libenv.add_plane(center::Float64_3, length_x::Float64, length_z::Float64, material_name::Cstring, rotation::Quaternion)::Filament_Entity_ID
end

function add_line(begin_point, end_point, material_name::CStaticString{N})::Filament_Entity_ID where N
    @ccall libenv.add_line(begin_point::Float64_3, end_point::Float64_3, material_name::Cstring)::Filament_Entity_ID
end

exists(filament_entity::Filament_Entity_ID)::Bool = @ccall libenv.filament_entity_exists(filament_entity::Filament_Entity_ID)::Bool
exists(gltf_instance::glTF_Instance_ID)::Bool = @ccall libenv.gltf_instance_exists(gltf_instance::glTF_Instance_ID)::Bool
    
# 
# Frame Handling
#
# The Frame is what the image gets rendered into. It is mainly a wrapper around Filaments SwapChain.
#

create_frame(env::Environment_ID, width, height)::Frame_ID = @ccall libenv.create_frame(env::Environment_ID, width::UInt32, height::UInt32)::Frame_ID
exists(frame::Frame_ID)::Bool = @ccall libenv.frame_exists(frame::Frame_ID)::Bool
destroy(frame::Frame_ID)::Bool = @ccall libenv.destroy_frame(frame::Frame_ID)::Bool

get_pixel_data(frame::Frame_ID, pixel_data::Ptr{Ptr{Cvoid}}, width::Ptr{UInt32}, height::Ptr{UInt32})::Bool = @ccall libenv.get_pixel_data(frame::Frame_ID, pixel_data::Ptr{Ptr{Cvoid}}, width::Ptr{UInt32}, height::Ptr{UInt32})::Bool
disable_pixel_capture(frame::Frame_ID)::Bool = @ccall libenv.disable_pixel_capture(frame::Frame_ID)::Bool

# 
# Camera Handling
#

function create_camera(env::Environment_ID;
                       pos = Float64_3(0.0, 0.0, 5.0),
                       lookat = Float64_3(0.0, 0.0, 0.0),
                       up = Float64_3(0.0, 1.0, 0.0),
                       vertical_fov = 60.0,
                       near_plane = 0.1,
                       far_plane = 50.0,
                       width = UInt32(800),
                       height = UInt32(600))::Camera_ID

    @ccall libenv.create_camera(env::Environment_ID,
                                pos::Float64_3,
                                lookat::Float64_3,
                                up::Float64_3,
                                vertical_fov::Float64,
                                near_plane::Float64,
                                far_plane::Float64,
                                width::UInt32,
                                height::UInt32)::Camera_ID
end
exists(camera::Camera_ID)::Bool = @ccall libenv.camera_exists(camera::Camera_ID)::Bool
destroy(camera::Camera_ID)::Bool = @ccall libenv.destroy_camera(camera::Camera_ID)::Bool

set_camera_fov_vertical(camera::Camera_ID, vertical_fov)::Bool = @ccall libenv.set_camera_fov_vertical(camera::Camera_ID, vertical_fov::Float64)::Bool
get_camera_fov_vertical(camera::Camera_ID)::Float64 = @ccall libenv.get_camera_fov_vertical(camera::Camera_ID)::Float64
get_camera_up_vector(camera::Camera_ID)::Float64_3 = @ccall libenv.get_camera_up_vector(camera::Camera_ID)::Float64_3
get_camera_forward_vector(camera::Camera_ID)::Float64_3 = @ccall libenv.get_camera_forward_vector(camera::Camera_ID)::Float64_3
render_frame(camera::Camera_ID, frame::Frame_ID)::Bool = @ccall libenv.render_frame(camera::Camera_ID, frame::Frame_ID)::Bool
get_filament_entity(camera_id::Camera_ID)::Filament_Entity_ID = @ccall libenv.get_camera_filament_entity(camera_id::Camera_ID)::Filament_Entity_ID

#
# Window Handling
#

"Create a new window and set it as the current active window."
function create_window(camera::Camera_ID, name::CStaticString{N}; target_fps = 60)::Window_ID where N
    @ccall libenv.create_window(camera::Camera_ID, target_fps::Int32, name::Cstring)::Window_ID
end
exists(window::Window_ID)::Bool = @ccall libenv.window_exists(window::Window_ID)::Bool
destroy(window::Window_ID)::Bool = @ccall libenv.destroy_window(window::Window_ID)::Bool

"Change the 'active window' to different window."
set_active_window(window::Window_ID)::Bool = @ccall libenv.set_active_window(window::Window_ID)::Bool

is_active_window_set()::Bool = @ccall libenv.is_active_window_set()::Bool

"Checking for events like keyboard input etc. and rendering the window."
update_window()::Bool = @ccall libenv.update_window()::Bool

"Get the time between the previous two frames of the active window."
get_last_frame_time_of_window_ms()::Float64 = @ccall libenv.get_last_frame_time_of_window_ms()::Float64

"Focus input from keyboard, mouse, joystick, etc. to the active window."
focus_input_to_window()::Bool = @ccall libenv.focus_input_to_window()::UInt8

#
# Window User Input
#

const ENV_MAX_MOUSE_BUTTONS = 8
const ENV_MAX_JOYSTICK_AXES = 8

# Keyboard

@enum SDL_Scancode begin
    SDL_SCANCODE_A = 4
    SDL_SCANCODE_D = 7
    SDL_SCANCODE_R = 21
    SDL_SCANCODE_S = 22
    SDL_SCANCODE_W = 26
    SDL_SCANCODE_SPACE = 44
end

is_key_down(code::SDL_Scancode)::Bool = @ccall libenv.is_key_down(code::SDL_Scancode)::Bool
is_key_pressed(code::SDL_Scancode)::Bool = @ccall libenv.is_key_pressed(code::SDL_Scancode)::Bool
is_key_up(code::SDL_Scancode)::Bool = @ccall libenv.is_key_up(code::SDL_Scancode)::Bool
is_key_released(code::SDL_Scancode)::Bool = @ccall libenv.is_key_released(code::SDL_Scancode)::Bool

# Mouse

@enum Mouse_Button begin
    MOUSE_BUTTON_NONE = 0
    MOUSE_BUTTON_LEFT = 1
    MOUSE_BUTTON_MIDDLE = 2
    MOUSE_BUTTON_RIGHT = 3
end

is_mouse_button_down(button::Mouse_Button)::Bool = @ccall libenv.is_mouse_button_down(button::Mouse_Button)::Bool
is_mouse_button_pressed(button::Mouse_Button)::Bool = @ccall libenv.is_mouse_button_pressed(button::Mouse_Button)::Bool
is_mouse_button_up(button::Mouse_Button)::Bool = @ccall libenv.is_mouse_button_up(button::Mouse_Button)::Bool
is_mouse_button_released(button::Mouse_Button)::Bool = @ccall libenv.is_mouse_button_released(button::Mouse_Button)::Bool
get_mouse_pos()::Int32_2 = @ccall libenv.get_mouse_pos()::Int32_2
get_mouse_delta()::Int32_2 = @ccall libenv.get_mouse_delta()::Int32_2
get_mouse_wheel_delta()::Int32_2 = @ccall libenv.get_mouse_wheel_delta()::Int32_2

#
# Entity Transformation
#

get_position(filament_entity::Filament_Entity_ID)::Float64_3 = @ccall libenv.get_position(filament_entity::Filament_Entity_ID)::Float64_3

set_position(filament_entity::Filament_Entity_ID, pos)::Bool = @ccall libenv.set_position(filament_entity::Filament_Entity_ID, pos::Float64_3)::Bool
set_orientation(filament_entity::Filament_Entity_ID, orientation::Quaternion)::Bool = @ccall libenv.set_orientation(filament_entity::Filament_Entity_ID, orientation::Quaternion)::Bool
set_position_and_orientation(filament_entity::Filament_Entity_ID, pos, orientation::Quaternion)::Bool = @ccall libenv.set_position_and_orientation(filament_entity::Filament_Entity_ID, pos::Float64_3, orientation::Quaternion)::Bool

#
# User Controllable Camera
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

"Dont use this function"
function update_camera_by_window_input_first_person!(camera::Camera_ID, state::Camera_Motion_State, zoom_sensitivity, max_vertical_fov, min_vertical_fov)
    # Zooming
    scroll_delta_effective = get_mouse_wheel_delta() .* zoom_sensitivity * get_last_frame_time_of_window_ms()
    prev_fov = get_camera_fov_vertical(camera)
    new_fov = prev_fov + prev_fov * scroll_delta_effective.y
    if (min_vertical_fov <= new_fov <= max_vertical_fov)
        set_camera_fov_vertical(camera, new_fov)
    end

    # Updating the camera
    set_position_and_orientation(get_filament_entity(camera), state.orbit_center, state.orientation)
end

"Dont use this function"
function update_camera_by_window_input_third_person!(camera::Camera_ID, state::Camera_Motion_State, zoom_sensitivity, max_center_distance, min_center_distance)
    # Zooming
    scroll_delta_effective = get_mouse_wheel_delta() .* zoom_sensitivity * get_last_frame_time_of_window_ms()
    new_center_distance = state.center_distance + state.center_distance * scroll_delta_effective.y
    if (min_center_distance <= new_center_distance <= max_center_distance)
        state.center_distance = new_center_distance
    end

    # Updating the camera
    set_orientation(get_filament_entity(camera), state.orientation)
    # The orientation needs to be updated first, so that the 'forward vector' we are "getting" is up to date,
    # otherwise there are laggy movement artifacts.
    set_position(get_filament_entity(camera), state.orbit_center - get_camera_forward_vector(camera) .* state.center_distance)
end

"""
Use the keyboard and mouse input to the current 'active window' to update the cameras position and orientation.
To use this you also need to provide a camera 'state' object, which should be maintained between calls.
"""
function update_camera_by_window_input!(camera::Camera_ID, state::Camera_Motion_State;
                                        mode::Camera_Motion_Mode = THIRD_PERSON,
                                        rotation_sensitivity = 0.001,
                                        translation_sensitivity = 0.003,
                                        zoom_sensitivity = 0.005,
                                        walk_speed = 0.5,
                                        max_vertical_fov = 100.0,
                                        min_vertical_fov = 5.0,
                                        max_center_distance = 10000.0,
                                        min_center_distance = 0.01)

    # Rotation
    if is_mouse_button_down(MOUSE_BUTTON_LEFT)
        mouse_delta_effective = -get_mouse_delta() .* rotation_sensitivity * get_last_frame_time_of_window_ms()
        
        orientation_delta_pitch = normed_axis_angle_to_quaternion(mouse_delta_effective.y, Float64_3(1.0, 0.0, 0.0)) # in local coordinate space
        orientation_delta_yaw = normed_axis_angle_to_quaternion(mouse_delta_effective.x, get_camera_up_vector(camera)) # in global coordinate space
        # The 'combine'-order is very important here.
        # First we pitch around the a vector pointing to the left of the camera in local coordinates (1,0,0),
        # then we rotate by the cameras previous orientation, and finally around the cameras 'up' vector.
        state.orientation = combine(combine(orientation_delta_pitch, state.orientation), orientation_delta_yaw)
    end
    
    # Walking
    if is_key_down(SDL_SCANCODE_W)
        state.orbit_center += get_camera_forward_vector(camera) .* (walk_speed / get_last_frame_time_of_window_ms())
    end
    if is_key_down(SDL_SCANCODE_S)
        state.orbit_center -= get_camera_forward_vector(camera) .* (walk_speed / get_last_frame_time_of_window_ms())
    end
    if is_key_down(SDL_SCANCODE_D)
        dir = norm_vector(cross_product(get_camera_forward_vector(camera), get_camera_up_vector(camera)))
        state.orbit_center += dir .* (walk_speed / get_last_frame_time_of_window_ms())
    end
    if is_key_down(SDL_SCANCODE_A)
        dir = norm_vector(cross_product(get_camera_forward_vector(camera), get_camera_up_vector(camera)))
        state.orbit_center -= dir .* (walk_speed / get_last_frame_time_of_window_ms())
    end

    # Paning
    if is_mouse_button_down(MOUSE_BUTTON_RIGHT)
        mouse_delta_effective = get_mouse_delta() .* (translation_sensitivity * state.center_distance)
        forward = get_camera_forward_vector(camera)
        up = get_camera_up_vector(camera)
        right = norm_vector(cross_product(up, forward))
        head_dir = norm_vector(cross_product(forward, right))
        state.orbit_center += right .* mouse_delta_effective.x + head_dir .* mouse_delta_effective.y
    end
    
    if mode == FIRST_PERSON
        update_camera_by_window_input_first_person!(camera, state, zoom_sensitivity, max_vertical_fov, min_vertical_fov)
    elseif mode == THIRD_PERSON
        update_camera_by_window_input_third_person!(camera, state, zoom_sensitivity, max_center_distance, min_center_distance)
    end
end

#
# Joystick
#

connect_to_joystick()::Bool = @ccall libenv.connect_to_joystick()::Bool
is_connected_to_joystick()::Bool = @ccall libenv.is_connected_to_joystick()::Bool
disconnect_from_joystick()::Bool = @ccall libenv.disconnect_from_joystick()::Bool
# axis_idx - 1 because c++ uses 0 based indexing
get_joystick_axis_raw(axis_idx)::Int16 = @ccall libenv.get_joystick_axis_raw((axis_idx - 1)::UInt8)::Int16

@enum Joystick_Axis begin
    JOYSTICK_NONE = 0
    JOYSTICK_THROTTLE = 1
    JOYSTICK_YAW = 2
    JOYSTICK_PITCH = 3
    JOYSTICK_ROLL = 4
end

# axis_idx - 1 because c++ uses 0 based indexing
"Internally SDL2 sees joystick axis as Uint8 indices, these indices should be associated to their actual role (Throttle, Yaw, etc.)"
assign_joystick_axis_idx_to_axis_type(axis_idx, axis_type::Joystick_Axis)::Bool = @ccall libenv.assign_joystick_axis_idx_to_axis_type((axis_idx - 1)::UInt8, axis_type::UInt8)::Bool

"The 'range' of the raw joystick values is required to map them to the range [-1, 1]"
set_joystick_axis_range(axis_type::Joystick_Axis, min, zero, max)::Bool = @ccall libenv.set_joystick_axis_range(axis_type::UInt8, min::Int16, zero::Int16, max::Int16)::Bool

"""
Returns the mapped value of the joystick. The output range is [-1.0, 1.0].
Because we map 'zero' exaclty, to ensures that the sticks resting position is actually at zero,
the output won't be smoove in 0, since we have two slightly different linear interpolations for > 0 and < 0.
"""
get_joystick_axis_mapped_value(axis_type::Joystick_Axis)::Float64 = @ccall libenv.get_joystick_axis_mapped_value(axis_type::UInt8)::Float64

"Dont use this directly"
function find_dominant_joystick_axis_and_its_max_range()::Tuple{UInt8, Int16, Int16} # axis_idx, min value, max value

    # create two Int16 vectors
    joystick_axes_raw_min = fill(Int16(2^15-1), ENV_MAX_JOYSTICK_AXES)
    joystick_axes_raw_max = fill(Int16(-2^15), ENV_MAX_JOYSTICK_AXES)

    # First find the raw min and max values for all axes
    start_time = time()
    while time() - start_time < 3.0 # collect joystick data for 3 seconds
        for axis_idx::UInt8 in 1:ENV_MAX_JOYSTICK_AXES
            raw = get_joystick_axis_raw(axis_idx)
            
            if raw < joystick_axes_raw_min[axis_idx]
                joystick_axes_raw_min[axis_idx] = raw
            elseif raw > joystick_axes_raw_max[axis_idx]
                joystick_axes_raw_max[axis_idx] = raw
            end
        end

        # Usually the window is not selected when this is called,
        # so we need to ensure that our window receives the joystick input.
        focus_input_to_window()
        update_window()
    end

    # Determine the axis, which has been moved the most
    most_moved_axis = 1;
    most_moved_delta = joystick_axes_raw_max[1] - joystick_axes_raw_min[1]
    for axis_idx::UInt8 in 2:ENV_MAX_JOYSTICK_AXES
        axis_moved_delta = joystick_axes_raw_max[axis_idx] - joystick_axes_raw_min[axis_idx]
        
        if axis_moved_delta > most_moved_delta
            most_moved_axis = axis_idx
            most_moved_delta = axis_moved_delta
        end
    end

    return (most_moved_axis, joystick_axes_raw_min[most_moved_axis], joystick_axes_raw_max[most_moved_axis])
end

"""
Performs a manual calibration routine and generates code for setting the determined calibration values.
Can be used in two ways:
1) Execute once and copy the generated code into your script.
2) Call with 'eval': `eval(generate_joystick_calibration_code())` to immediately appy the calibration.
"""
function generate_joystick_calibration_code()::Expr

    if !(is_active_window_set())
        println("ERROR: You need to create a Window so that we have a means to receive the joystick input.")
        return Expr()
    end

    if !(is_connected_to_joystick())
        println("ERROR: Connect to joystick before performing the manual calibration.")
        return Expr()
    end
    
    println("Move THROTTLE up and down, reaching min and max.");
    sleep(0.5) # this delay accounts for human reaction time
    throttle_axis_idx, throttle_raw_value_min, throttle_raw_value_max = find_dominant_joystick_axis_and_its_max_range()
    println("Detected throttle on axis '", throttle_axis_idx, "' with min = ", throttle_raw_value_min, ", and max = ", throttle_raw_value_max)

    println("Move YAW left and right, reaching min and max.");
    sleep(0.5)
    yaw_axis_idx, yaw_raw_value_min, yaw_raw_value_max = find_dominant_joystick_axis_and_its_max_range()
    println("Detected yaw on axis '", yaw_axis_idx, "' with min = ", yaw_raw_value_min, ", and max = ", yaw_raw_value_max)

    println("Move PITCH up and down, reaching min and max.");
    sleep(0.5)
    pitch_axis_idx, pitch_raw_value_min, pitch_raw_value_max = find_dominant_joystick_axis_and_its_max_range()
    println("Detected pitch on axis '", pitch_axis_idx, "' with min = ", pitch_raw_value_min, ", and max = ", pitch_raw_value_max)

    println("Move ROLL left and right, reaching min and max.");
    sleep(0.5)
    roll_axis_idx, roll_raw_value_min, roll_raw_value_max = find_dominant_joystick_axis_and_its_max_range()
    println("Detected roll on axis '", roll_axis_idx, "' with min = ", roll_raw_value_min, ", and max = ", roll_raw_value_max)

    println("Now CENTER all your sticks.");
    sleep(0.5)

    throttle_raw_zero = 0.0
    yaw_raw_zero = 0.0
    pitch_raw_zero = 0.0
    roll_raw_zero = 0.0

    start_time = time()
    while time() - start_time < 4.0 # collect joystick data for 4 seconds
        # We are using an Exponential Moving Average to find the 'zero' values.
        α = 0.1
        throttle_raw_zero = α * get_joystick_axis_raw(throttle_axis_idx) + (1 - α) * throttle_raw_zero
        yaw_raw_zero = α * get_joystick_axis_raw(yaw_axis_idx) + (1 - α) * yaw_raw_zero
        pitch_raw_zero = α * get_joystick_axis_raw(pitch_axis_idx) + (1 - α) * pitch_raw_zero
        roll_raw_zero = α * get_joystick_axis_raw(roll_axis_idx) + (1 - α) * roll_raw_zero

        # Usually the window is not selected when this is called,
        # so we need to ensure that our window receives the joystick input.
        focus_input_to_window()
        update_window()
    end
    
    println("All Done!")
    
    return quote
        if(!TinyDronesSim.Environments.is_connected_to_joystick())
            println("Connect to joystick before executing the calibration routine.")
        else
            TinyDronesSim.Environments.assign_joystick_axis_idx_to_axis_type($throttle_axis_idx, TinyDronesSim.Environments.JOYSTICK_THROTTLE)
            TinyDronesSim.Environments.set_joystick_axis_range(TinyDronesSim.Environments.JOYSTICK_THROTTLE, $throttle_raw_value_min, $(round(Int16, throttle_raw_zero)), $throttle_raw_value_max)
            
            TinyDronesSim.Environments.assign_joystick_axis_idx_to_axis_type($yaw_axis_idx, TinyDronesSim.Environments.JOYSTICK_YAW)
            TinyDronesSim.Environments.set_joystick_axis_range(TinyDronesSim.Environments.JOYSTICK_YAW, $yaw_raw_value_min, $(round(Int16, yaw_raw_zero)), $yaw_raw_value_max)
            
            TinyDronesSim.Environments.assign_joystick_axis_idx_to_axis_type($pitch_axis_idx, TinyDronesSim.Environments.JOYSTICK_PITCH)
            TinyDronesSim.Environments.set_joystick_axis_range(TinyDronesSim.Environments.JOYSTICK_PITCH, $pitch_raw_value_min, $(round(Int16, pitch_raw_zero)), $pitch_raw_value_max)
            
            TinyDronesSim.Environments.assign_joystick_axis_idx_to_axis_type($roll_axis_idx, TinyDronesSim.Environments.JOYSTICK_ROLL)
            TinyDronesSim.Environments.set_joystick_axis_range(TinyDronesSim.Environments.JOYSTICK_ROLL, $roll_raw_value_min, $(round(Int16, roll_raw_zero)), $roll_raw_value_max)
        end
    end
end

