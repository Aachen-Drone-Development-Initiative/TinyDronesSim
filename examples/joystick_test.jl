using TinyDronesSim
using StaticStrings
const Env = TinyDronesSim.Environments

env = Env.create_environment()
camera = Env.create_camera(env)
window = Env.create_window(camera, cstatic"joystick test")

Env.connect_to_joystick()

eval(Env.generate_joystick_calibration_code())

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
