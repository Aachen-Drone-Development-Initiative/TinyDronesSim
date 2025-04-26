using TinyDronesSim
using StaticStrings
const Env = TinyDronesSim.Environments

env = Env.create_environment()

camera = Env.create_camera(env)
camera_motion_state = Env.Camera_Motion_State()

window = Env.create_window(camera, cstatic"joystick test", target_fps = Int32(60))

Env.connect_to_joystick()

# generated calibration for my rc-radio

# call 'Env.generate_joystick_calibration_code()'
# to generate this for your controller/joystick
# you can also call 'eval(Env.generate_joystick_calibration_code())' to immediately execute the generated calibration code

if !(TinyDronesSim.Environments.is_connected_to_joystick())
    println("Connect to joystick before executing the calibration routine.")
    return
end
TinyDronesSim.Environments.assign_joystick_axis_idx_to_axis_type(UInt8(0x03), TinyDronesSim.Environments.JOYSTICK_THROTTLE)
TinyDronesSim.Environments.set_joystick_axis_range(TinyDronesSim.Environments.JOYSTICK_THROTTLE, Int16(-19325), Int16(2184))
TinyDronesSim.Environments.assign_joystick_axis_idx_to_axis_type(UInt8(0x05), TinyDronesSim.Environments.JOYSTICK_YAW)
TinyDronesSim.Environments.set_joystick_axis_range(TinyDronesSim.Environments.JOYSTICK_YAW, Int16(-19325), Int16(2184))
TinyDronesSim.Environments.assign_joystick_axis_idx_to_axis_type(UInt8(0x02), TinyDronesSim.Environments.JOYSTICK_PITCH)
TinyDronesSim.Environments.set_joystick_axis_range(TinyDronesSim.Environments.JOYSTICK_PITCH, Int16(-19325), Int16(2184))
TinyDronesSim.Environments.assign_joystick_axis_idx_to_axis_type(UInt8(0x01), TinyDronesSim.Environments.JOYSTICK_ROLL)
TinyDronesSim.Environments.set_joystick_axis_range(TinyDronesSim.Environments.JOYSTICK_ROLL, Int16(-19325), Int16(2184))

# end generated calibration

i = 0
while Env.exists(window)
    if i % 10 == 0
        print("throttle: ", Env.get_joystick_axis_mapped_value(Env.JOYSTICK_THROTTLE))
        print(" yaw: ", Env.get_joystick_axis_mapped_value(Env.JOYSTICK_YAW))
        print(" pitch: ", Env.get_joystick_axis_mapped_value(Env.JOYSTICK_PITCH))
        println(" roll: ", Env.get_joystick_axis_mapped_value(Env.JOYSTICK_ROLL))
    end
    global i+=1
    
    Env.update_window()
end

Env.destroy_everything()
