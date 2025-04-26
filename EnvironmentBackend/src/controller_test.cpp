#include <SDL2/SDL.h>
#include <cstdio>

int main() {
    if (SDL_Init(SDL_INIT_JOYSTICK) < 0) {
        printf("Failed to initialize SDL: %s\n", SDL_GetError());
        return 1;
    }

    SDL_JoystickEventState(SDL_ENABLE);

    if (SDL_NumJoysticks() < 1) {
        printf("No joysticks connected!\n");
        SDL_Quit();
        return 1;
    }

    SDL_Joystick *joystick = SDL_JoystickOpen(0);
    if (!joystick) {
        printf("Could not open joystick: %s\n", SDL_GetError());
        SDL_Quit();
        return 1;
    }

    printf("Joystick Name: %s\n", SDL_JoystickName(joystick));
    printf("Axes: %d\n", SDL_JoystickNumAxes(joystick));
    printf("Buttons: %d\n", SDL_JoystickNumButtons(joystick));

    SDL_Event e;
    int running = 1;
    while (running) {
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_JOYAXISMOTION) {
                printf("Axis %d moved to %d\n", e.jaxis.axis, e.jaxis.value);
            } else if (e.type == SDL_JOYBUTTONDOWN || e.type == SDL_JOYBUTTONUP) {
                printf("Button %d %s\n", e.jbutton.button, e.jbutton.state ? "pressed" : "released");
            } else if (e.type == SDL_QUIT) {
                running = 0;
            }
        }
        SDL_Delay(10); // Give CPU a break
    }

    SDL_JoystickClose(joystick);
    SDL_Quit();
    return 0;
}
