
@kwdef mutable struct Camera_Motion_State
    orbit_center::Float64_3 = Float64_3(0.0, 0.0, 0.0)
    center_distance::Float64 = 1.0 # only used for THIRD_PERSON mode
    orientation::Quaternion = quat_identity() # we need to store this because it's hard to convert matrices to quaternions
end

@enum Camera_Motion_Mode begin
    FIRST_PERSON
    THIRD_PERSON
end

"""
Use the keyboard and mouse input of the current viewport to update the cameras position and orientation.
This fuction operates on a the camera as well as a state object, where additional like the distance from the center is stored.
"""
function camera_update_by_user_input!(camera::Camera_ID,
                                      state::Camera_Motion_State,
                                      delta_time_us::UInt64,
                                      delta_mouse_wheel::Int32,
                                      delta_mouse_pos::Int32_2;
                                      
                                      mode::Camera_Motion_Mode = THIRD_PERSON,
                                      rotation_sensitivity = 1,
                                      translation_sensitivity = 3,
                                      zoom_sensitivity = 0.01,
                                      walk_speed = 0.001,
                                      max_vertical_fov = 100.0,
                                      min_vertical_fov = 5.0,
                                      max_center_distance = 10000.0,
                                      min_center_distance = 0.01)

    dt_s::Float64 = delta_time_us / 1e6

    # Rotation
    if viewport_mouse_button_down(MOUSE_BUTTON_LEFT)
        mouse_delta_effective = -delta_mouse_pos .* rotation_sensitivity * dt_s

        orientation_delta_pitch = quat_from_normed_axis_angle(mouse_delta_effective.y, Float64_3(1.0, 0.0, 0.0)) # in local coordinate space
        orientation_delta_yaw = quat_from_normed_axis_angle(mouse_delta_effective.x, camera_up_vector(camera)) # in global coordinate space
        # The 'combine'-order is very important here.
        # First we pitch around the a vector pointing to the left of the camera in local coordinates (1,0,0),
        # then we rotate by the cameras previous orientation, and finally around the cameras 'up' vector.
        state.orientation = quat_combine(quat_combine(orientation_delta_pitch, state.orientation), orientation_delta_yaw)
    end
    
    # Walking
    if viewport_key_down('w')
        state.orbit_center += camera_forward_vector(camera) .* (walk_speed / dt_s)
    end
    if viewport_key_down('s')
        state.orbit_center -= camera_forward_vector(camera) .* (walk_speed / dt_s)
    end
    if viewport_key_down('d')
        dir = norm_vector(vec_cross(camera_forward_vector(camera), camera_up_vector(camera)))
        state.orbit_center += dir .* (walk_speed / dt_s)
    end
    if viewport_key_down('a')
        dir = norm_vector(vec_cross(camera_forward_vector(camera), camera_up_vector(camera)))
        state.orbit_center -= dir .* (walk_speed / dt_s)
    end

    # Paning
    if viewport_mouse_button_down(MOUSE_BUTTON_RIGHT)
        mouse_delta_effective = delta_mouse_pos .* (translation_sensitivity * state.center_distance)
        forward = camera_forward_vector(camera)
        up = camera_up_vector(camera)
        right = vec_normalize(vec_cross(up, forward))
        head_dir = vec_normalize(vec_cross(forward, right))
        state.orbit_center += right .* mouse_delta_effective.x + head_dir .* mouse_delta_effective.y
    end
    
    if mode == FIRST_PERSON
        # Zooming
        scroll_delta_effective = delta_mouse_wheel * zoom_sensitivity * dt_s
        prev_fov = camera_fov_vertical(camera)
        new_fov = prev_fov + prev_fov * scroll_delta_effective
        
        # Updating the camera
        if (min_vertical_fov <= new_fov <= max_vertical_fov)
            set_camera_fov_vertical(camera, new_fov)
        end
        camera_set_position_and_orientation(camera, state.orbit_center, state.orientation)
    elseif mode == THIRD_PERSON
        # Zooming
        scroll_delta_effective = delta_mouse_wheel * zoom_sensitivity * dt_s
        next_center_distance = state.center_distance + state.center_distance * scroll_delta_effective
        if min_center_distance <= next_center_distance <= max_center_distance
            state.center_distance = next_center_distance
        end

        # Updating the camera
        camera_set_orientation(camera, state.orientation)
        # The orientation needs to be updated first, so that the 'forward vector' we are "getting" is up to date,
        # otherwise there are laggy movement artifacts.
        camera_set_position(camera, state.orbit_center - camera_forward_vector(camera) .* state.center_distance)
    end
end
