using TinyDronesSim
const tds = TinyDronesSim

env = tds.env_create()
camera = tds.camera_create(env)
viewport = tds.viewport_create()

# TODO: serial port
serialport = tds.serialport_create_and_connect(baudrate=115200, port="/dev/ttyACM0")

struct IMU_Data
    acceleration::tds.Float32_3
    omega::tds.Float32_3
end

tds.serialport_define_receive_object(IMU_Data)

# stuff for visualization
tds.env_add_unlit_material("red_unlit", color=[1, 0, 0])
tds.env_add_unlit_material("green_unlit", color=[0, 1, 0])
acceleration_vector = tds.env_add_arrow(0.1, 1, "red_unlit")
omega_vector = tds.env_add_arrow(0.1, 1, "gree_unlit")

viewport_frame = tds.viewport_add_camera(viewport, camera)

# TODO: Make plots 'stackable' (multiple plots in one)
plot_accel_x = tds.viewport_add_plot(viewport, location_path="right")
plot_accel_y = tds.viewport_add_plot(viewport, location_path="right/down")
plot_accel_z = tds.viewport_add_plot(viewport, location_path="right/down/down")

plot_omega_x = tds.viewport_add_plot(viewport, location_path="right", location_name="acceleration|omega")
plot_omega_y = tds.viewport_add_plot(viewport, location_path="right/omega/down")
plot_omega_z = tds.viewport_add_plot(viewport, location_path="right/omega/down/down")

# update callback
camera_motion_state = tds.Camera_Motion_State()
function camera_update_callback(update::tds.Viewport_Frame_Callback_Update)::Int32

    imu_data_vec = Vector{IMU_Data}()
    while tds.serialport_queue_not_empty()
        push!(imu_data_vec, tds.serialport_next_object(serialport)::IMU_Data)
    end

    # TODO: maybe something like [imu_data_vec[].acceleration.x] acutally exists?
    tds.viewport_plot_append_numbers(plot_accel_x, [imu_data_vec[].acceleration.x])
    tds.viewport_plot_append_numbers(plot_accel_y, [imu_data_vec[].acceleration.y])
    tds.viewport_plot_append_numbers(plot_accel_z, [imu_data_vec[].acceleration.z])

    tds.viewport_plot_append_numbers(plot_omega_x, [imu_data_vec[].omega.x])
    tds.viewport_plot_append_numbers(plot_omega_y, [imu_data_vec[].omega.y])
    tds.viewport_plot_append_numbers(plot_omega_z, [imu_data_vec[].omega.z])

    # TODO: arrow updating from vector
    tds.arrow_entity_set_vector(acceleration_vector, imu_data_vec[end].acceleration)
    tds.arrow_entity_set_vector(omega_vector, imu_data_vec[end].omega)
    
    # update the camera
    tds.viewport_frame_update_camera_by_user_input!(update.viewport_frame,
                                                    update.camera,
                                                    camera_motion_state,
                                                    update.delta_time_us,
                                                    update.delta_mouse_wheel,
                                                    update.delta_mouse_drag)
    return 0
end

# TODO: implement callback updating
tds.viewport_frame_set_callback(viewport_frame, camera_update_callback)
