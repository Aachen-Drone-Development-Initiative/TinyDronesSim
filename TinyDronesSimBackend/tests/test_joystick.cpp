#include "../tinydronessim_backend.hpp"
#include <iostream>
#include <chrono>
#include <thread>

int main()
{
    Joystick_ID joystick = joystick_create_and_connect("/dev/input/js1");
    
    while (joystick_is_connected(joystick)) {

        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        
        Joystick_Raw raw = joystick_raw(joystick);

        if (raw.axes_length) {
            std::cout << "axes: ";
            for (int i = 0; i < raw.axes_length; ++i) {
                std::cout << raw.axes[i] << ", ";
            }
            std::cout << '\n';
        }

        if (raw.buttons_length) {
            std::cout << "buttons: ";
            for (int i = 0; i < raw.buttons_length; ++i) {
                std::cout << raw.buttons[i] << ", ";
            }
            std::cout << '\n';
        }
    }
}
