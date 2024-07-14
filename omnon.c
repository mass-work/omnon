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

#include <math.h>
#include "omnon.h"
#include "drivers/pmw33xx_common.c"
#include "config.h"
#include "timer.h"

#ifdef CONSOLE_ENABLE
    int8_t debugPMW3360x, debugPMW3360y = 0;
#endif
#ifdef POINTING_DEVICE_ENABLE

    #if JOYSTICK_MODE == 0
        #include "analog.h"
        #include "drivers/analog_joystick.h"

        uint16_t time_difference2(uint16_t t1, uint16_t t2){
            return (t1 - t2);
        }

        // 各ジョイスティック軸のデータを初期化
        JoystickAxisData lxData = {0, 0, 0, false};
        JoystickAxisData lyData = {0, 0, 0, false};
        JoystickAxisData rxData = {0, 0, 0, false};
        JoystickAxisData ryData = {0, 0, 0, false};

        int16_t lxOrigin, lyOrigin, rxOrigin, ryOrigin;
        int8_t  JoystickAdjustmentFactor = 8;
        uint16_t current_time = 0;
        uint32_t initialScrollPeriod = 500; // 最初の1秒間のスクロールタイミング
        uint32_t repeatScrollPeriod = 1900;   // 1秒経過後のスクロール頻度の基準
        uint32_t initialScrollTime = 0;

        void pointing_device_driver_init(void) {
            analog_joystick_init();
        }

        void process_joystick_axis_report(JoystickAxisData *axisData, int16_t direction, report_mouse_t *mouse_report, int8_t *mouse_report_axis, int8_t mouse_value_positive, int8_t mouse_value_negative) {
            if (abs(axisData->joyData) < 50) {
                axisData->initialScrollDone = false;
            } else {
                if (!axisData->initialScrollDone) {
                    if (direction > 0) {
                        *mouse_report_axis = mouse_value_positive;
                        axisData->lastTime1 = current_time;
                        axisData->initialScrollDone = true;
                    } else if (direction < 0) {
                        *mouse_report_axis = mouse_value_negative;
                        axisData->lastTime1 = current_time;
                        axisData->initialScrollDone = true;
                    }
                } else {
                    if (time_difference2(current_time, axisData->lastTime1) < initialScrollPeriod) {
                        axisData->lastTime2 = current_time;
                    } else {
                        if (time_difference2(current_time, axisData->lastTime2) > (repeatScrollPeriod / 2 / (direction * (direction / JoystickAdjustmentFactor)))) {
                            *mouse_report_axis = (direction > 0) ? mouse_value_positive : mouse_value_negative;
                            axisData->lastTime2 = current_time;
                        }
                    }
                }
            }
        }

        void process_joystick_axis_tap(JoystickAxisData *axisData, int16_t direction, uint8_t key_code_positive, uint8_t key_code_negative) {
            if (abs(axisData->joyData) < 50) {
                axisData->initialScrollDone = false;
            } else {
                if (!axisData->initialScrollDone) {
                    if (direction > 0) {
                        tap_code(key_code_positive);
                        axisData->lastTime1 = current_time;
                        axisData->initialScrollDone = true;
                    } else if (direction < 0) {
                        tap_code(key_code_negative);
                        axisData->lastTime1 = current_time;
                        axisData->initialScrollDone = true;
                    }
                } else {
                    if (time_difference2(current_time, axisData->lastTime1) < initialScrollPeriod) {
                        axisData->lastTime2 = current_time;
                    } else {
                        if (time_difference2(current_time, axisData->lastTime2) > (repeatScrollPeriod / (direction * (direction / JoystickAdjustmentFactor)))) {
                            tap_code((direction > 0) ? key_code_positive : key_code_negative);
                            axisData->lastTime2 = current_time;
                        }
                    }
                }
            }
        }

        report_mouse_t pointing_device_driver_get_report(report_mouse_t mouse_report) {
            report_analog_joystick_t data = analog_joystick_read();
            lxData.joyData = analogReadPin(ANALOG_JOYSTICK_LX_AXIS_PIN) - lxOrigin;
            lyData.joyData = analogReadPin(ANALOG_JOYSTICK_LY_AXIS_PIN) - lyOrigin;
            rxData.joyData = analogReadPin(ANALOG_JOYSTICK_RX_AXIS_PIN) - rxOrigin;
            ryData.joyData = analogReadPin(ANALOG_JOYSTICK_RY_AXIS_PIN) - ryOrigin;
            current_time = timer_read();

            process_joystick_axis_report(&lxData, data.lx, &mouse_report, &mouse_report.v, -1, 1);
            process_joystick_axis_report(&lyData, data.ly, &mouse_report, &mouse_report.h, 1, -1);
            process_joystick_axis_tap(&rxData, data.rx, KC_DOWN, KC_UP);
            process_joystick_axis_tap(&ryData, data.ry, KC_RIGHT, KC_LEFT);

            return mouse_report;
        }
    #endif

    void pointing_device_init_kb(void) {
        pmw33xx_init(0);         // index 1 is the fast device.
        pmw33xx_init(1);         // index 1 is the second device.a
        pmw33xx_set_cpi(0, 1000); // applies to first sensor
        pmw33xx_set_cpi(1, 1000); // applies to second sensor
        pointing_device_init_user();
    }

    // Contains report from sensor #0 already, need to merge in from sensor #1
    report_mouse_t pointing_device_task_kb(report_mouse_t mouse_report) {
        pmw33xx_report_t report0 = pmw33xx_read_burst(0);
        pmw33xx_report_t report1 = pmw33xx_read_burst(1);
        if (!report0.motion.b.is_lifted && report0.motion.b.is_motion) {
        // left
            #define constrain_hid(amt) ((amt) < -127 ? -127 : ((amt) > 127 ? 127 : (amt)))
            #ifdef CONSOLE_ENABLE
                debugPMW3360x = report0.delta_x;
                debugPMW3360y = report0.delta_y;
            #endif
            mouse_report.x = constrain_hid(mouse_report.x + report0.delta_x);
            mouse_report.y = constrain_hid(mouse_report.y + -report0.delta_y);
        }
        if (!report1.motion.b.is_lifted && report1.motion.b.is_motion) {
        // right
            #define constrain_hid(amt) ((amt) < -127 ? -127 : ((amt) > 127 ? 127 : (amt)))
            mouse_report.x = constrain_hid(mouse_report.x + -report1.delta_x);
            mouse_report.y = constrain_hid(mouse_report.y + report1.delta_y);
        }

        return pointing_device_task_user(mouse_report);
    }


#endif




