/* Copyright 2020 Christopher Courtney, aka Drashna Jael're  (@drashna) <drashna@live.com>
 *           2024 mass
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#if JOYSTICK_MODE == 0
    #include "quantum.h"
    #include "../omnon.h"
    #include "../config.h"
    #include "analog_joystick.h"
    #include "analog.h"
    #include "gpio.h"
    #include "wait.h"
    #include "timer.h"
    #include <stdlib.h>
    #include <math.h>

    // Set Parameters
    uint16_t minAxisValue = ANALOG_JOYSTICK_AXIS_MIN;
    uint16_t maxAxisValue = ANALOG_JOYSTICK_AXIS_MAX;

    uint8_t maxCursorSpeed = ANALOG_JOYSTICK_SPEED_MAX;
    uint8_t speedRegulator = ANALOG_JOYSTICK_SPEED_REGULATOR; // Lower Values Create Faster Movement

    int16_t lxOrigin, lyOrigin, rxOrigin, ryOrigin, position;

    uint16_t lastCursor = 0;

    int16_t axisCoordinate(pin_t pin, uint16_t origin) {
        int8_t  direction;
        int16_t distanceFromOrigin;
        int16_t range;

        position = analogReadPin(pin);

        if (origin == position) {
            return 0;
        } else if (origin > position) {
            distanceFromOrigin = origin - position;
            range              = origin - minAxisValue;
            direction          = -1;
        } else {
            distanceFromOrigin = position - origin;
            range              = maxAxisValue - origin;
            direction          = 1;
        }

        float   percent    = (float)distanceFromOrigin / range;
        int16_t coordinate = (int16_t)(percent * 100);
        // 傾きを付ける
        coordinate = coordinate - (coordinate*0.05) * (coordinate*0.045);
        if (coordinate < 0) {
            return 0;
        } else if (coordinate > 100) {
            return 100 * direction;
        } else {
            return coordinate * direction;
        }
    }

    int8_t axisToMouseComponent(pin_t pin, int16_t origin, uint8_t maxSpeed) {
        int16_t coordinate = axisCoordinate(pin, origin);
        if (coordinate != 0) {
            float percent = (float)coordinate / 100;
            return percent * maxCursorSpeed * (abs(coordinate) / speedRegulator);
        } else {
            return 0;
        }
    }

    report_analog_joystick_t analog_joystick_read(void) {
        report_analog_joystick_t report = {0};

        if (timer_elapsed(lastCursor) > ANALOG_JOYSTICK_READ_INTERVAL) {
            lastCursor = timer_read();
            report.lx = axisToMouseComponent(ANALOG_JOYSTICK_LX_AXIS_PIN, lxOrigin, maxCursorSpeed);
            report.ly = axisToMouseComponent(ANALOG_JOYSTICK_LY_AXIS_PIN, lyOrigin, maxCursorSpeed);
            report.rx = axisToMouseComponent(ANALOG_JOYSTICK_RX_AXIS_PIN, rxOrigin, maxCursorSpeed);
            report.ry = axisToMouseComponent(ANALOG_JOYSTICK_RY_AXIS_PIN, ryOrigin, maxCursorSpeed);
        }
    #ifdef ANALOG_JOYSTICK_CLICK_PIN
        report.button = !readPin(ANALOG_JOYSTICK_CLICK_PIN);
    #endif
        return report;
    }

    void analog_joystick_init(void) {
    #ifdef ANALOG_JOYSTICK_CLICK_PIN
        setPinInputHigh(ANALOG_JOYSTICK_CLICK_PIN);
    #endif
        // Account for drift
        lxOrigin = analogReadPin(ANALOG_JOYSTICK_LX_AXIS_PIN);
        lyOrigin = analogReadPin(ANALOG_JOYSTICK_LY_AXIS_PIN);
        rxOrigin = analogReadPin(ANALOG_JOYSTICK_RX_AXIS_PIN);
        ryOrigin = analogReadPin(ANALOG_JOYSTICK_RY_AXIS_PIN);
    }

#endif

