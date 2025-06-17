using TinyDronesSim
const tds = TinyDronesSim

joystick = tds.joystick_create_and_connect("/dev/input/js1")
tds.joystick_calibrate(joystick)

while true
    data = tds.joystick_data(joystick)
    println("throttle: ", data.throttle, " yaw: ", data.yaw, " pitch: ", data.pitch, " roll: ", data.roll);
    sleep(0.1)
end
