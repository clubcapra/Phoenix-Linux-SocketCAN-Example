#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wmissing-noreturn"
#define Phoenix_No_WPI // remove WPI dependencies

#include "ctre/Phoenix.h"
#include "ctre/phoenix/platform/Platform.h"
#include "ctre/phoenix/unmanaged/Unmanaged.h"
#include <string>
#include <iostream>
#include <chrono>
#include <thread>
#include <SDL2/SDL.h>
#include <unistd.h>

using namespace ctre::phoenix;
using namespace ctre::phoenix::platform;
using namespace ctre::phoenix::motorcontrol;
using namespace ctre::phoenix::motorcontrol::can;

/* make some talons for drive train */
TalonSRX talLeft(11);
TalonSRX talRght(12);

/** simple wrapper for code cleanup */
void sleepApp(int ms) {
    std::this_thread::sleep_for(std::chrono::milliseconds(ms));
}

int main() {
    /* don't bother prompting, just use can0 */
    //std::cout << "Please input the name of your can interface: ";
    std::string interface = "can0";
    //std::cin >> interface;
    ctre::phoenix::platform::can::SetCANInterface(interface.c_str());

    /* setup drive */
    talRght.SetInverted(true);

    while (true) {
        /* we are looking for gamepad (first time or after disconnect),
            neutral drive until gamepad (re)connected. */


        // wait for gamepad
        printf("Waiting for gamepad...\n");
        while (true) {
            /* SDL seems somewhat fragile, shut it down and bring it up */
            SDL_Quit();
            SDL_Init(SDL_INIT_JOYSTICK);

            /* poll for gamepad */
            int res = SDL_NumJoysticks();
            if (res > 0) { break; }
            if (res < 0) { printf("Err = %i\n", res); }

            /* yield for a bit */
            sleepApp(20);
        }
        printf("Waiting for gamepad...Found one\n");

        // Open the joystick for reading and store its handle in the joy variable
        SDL_Joystick *joy = SDL_JoystickOpen(0);
        if (joy == NULL) {
            /* back to top of while loop */
            continue;
        }

        // Get information about the joystick
        const char *name = SDL_JoystickName(joy);
        const int num_axes = SDL_JoystickNumAxes(joy);
        const int num_buttons = SDL_JoystickNumButtons(joy);
        const int num_hats = SDL_JoystickNumHats(joy);
        printf("Now reading from joystick '%s' with:\n"
               "%d axes\n"
               "%d buttons\n"
               "%d hats\n\n",
               name,
               num_axes,
               num_buttons,
               num_hats);

        /* I'm using a logitech F350 wireless in D mode.
        If num axis is 6, then gamepad is in X mode, so neutral drive and wait for D mode.
        [SAFETY] This means 'X' becomes our robot-disable button.
        This can be removed if that's not the goal. */
        if (num_axes >= 6) {
            /* back to top of while loop */
            continue;
        }

        // Keep reading the state of the joystick in a loop
        while (true) {
            /* poll for disconnects or bad things */
            SDL_Event event;
            if (SDL_PollEvent(&event)) {
                if (event.type == SDL_QUIT) { break; }
                if (event.jdevice.type == SDL_JOYDEVICEREMOVED) { break; }
            }

            /* grab some stick values */
            double left_y_axe = ((double) SDL_JoystickGetAxis(joy, 1)) / -32767.0;
            double right_y_axe = ((double) SDL_JoystickGetAxis(joy, 3)) / -32767.0;

            // Uncomment for debug
//            printf("left track : %f \n",left_y_axe);
//            printf("right track : %f \n",right_y_axe);
            talLeft.Set(ControlMode::PercentOutput,left_y_axe);
            talRght.Set(ControlMode::PercentOutput,right_y_axe);


            /* [SAFETY] only enable drive if top left shoulder button is held down */
            if (SDL_JoystickGetButton(joy, 7)) {
//                printf("testing feed enable \n");
                ctre::phoenix::unmanaged::FeedEnable(100);
            }

            /* loop yield for a bit */
            sleepApp(20);
        }

        /* we've left the loop, likely due to gamepad disconnect */

        SDL_JoystickClose(joy);
        printf("gamepad disconnected\n");
    }

    SDL_Quit();
    return 0;
}

#pragma clang diagnostic pop