/*
Copyright 2024 mass

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#pragma once

/* SPI & PMW3360 settings. */
#define SPI_DRIVER SPID0
#define SPI_SCK_PIN GP2
#define SPI_MISO_PIN GP4
#define SPI_MOSI_PIN GP3
#define PMW33XX_CS_PINS { GP1, GP5 }
#define PMW33XX_CLOCK_SPEED 2000000

#define JOYSTICK_MODE 0  // 0: HIDデバイスモード, 1: Gamepadモード

#if JOYSTICK_MODE == 0
    // HIDデバイスモード
    #define ANALOG_JOYSTICK_LY_AXIS_PIN GP28
    #define ANALOG_JOYSTICK_LX_AXIS_PIN GP29
    #define ANALOG_JOYSTICK_RY_AXIS_PIN GP26
    #define ANALOG_JOYSTICK_RX_AXIS_PIN GP27
#elif JOYSTICK_MODE == 1
    // Gamepadモード
    #define JOYSTICK_BUTTON_COUNT 16
    #define JOYSTICK_AXIS_COUNT 6
    #define JOYSTICK_AXIS_RESOLUTION 10
#endif


