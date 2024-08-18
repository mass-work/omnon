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
#include "keymaps/default/custom_action.c"

#ifdef CONSOLE_ENABLE
    int8_t debugPMW3360x, debugPMW3360y, debugPMW3360xy = 0;
#endif

#define ANGLE_THRESHOLD 1
#define TB_MOVE_ANGLE_THRESHOLD 45
#define XSCALE_FACTOR 4
#define YSCALE_FACTOR 4
#define X_SC_SCALE_FACTOR 30
#define Y_SC_SCALE_FACTOR 30
#define MA_WINDOW_SIZE 10 //移動平均のサイズ
#define MOVING_AVERAGE_SIZE 10 // 移動平均のサンプルサイズ
#define INACTIVITY_TIMEOUT 100 // msec無操作でリセット
#define TB_RETURN_TIME 20 // msec無操作でリセット
#define TB_MOUSE_BOTTON_PUSH_TIME 200 // msec以上経過しないとボタンを押し直さない

bool button3_pressed = false;
bool shift_pressed = false;
// bool ctrl_pressed = false;
bool left_motion_detected, right_motion_detected, dual_motion_detected = false;
int8_t tb_left_orient, tb_right_orient = 0;
uint16_t tb_last_left_motion_time,tb_last_right_motion_time = 0;
uint16_t tb_current_time = 0;
uint16_t dual_init_time = 0;
uint16_t left_init_time = 0;
uint16_t dual_motion_time = 0;
uint16_t motion_gesture_time = 20;
int16_t left_ball_move_angle = 0;
int16_t right_ball_move_angle = 0;

uint16_t time_difference2(uint16_t t1, uint16_t t2){
    return (t1 - t2);
}

// スクロール
#if TRACKBALL_MODE == 0
    static float accumulated_h = 0.0f;
    static float accumulated_v = 0.0f;

    // 移動平均のためのバッファとインデックス
    float delta_x_buffer[MOVING_AVERAGE_SIZE] = {0};
    float delta_y_buffer[MOVING_AVERAGE_SIZE] = {0};
    int buffer_index = 0;

    // 移動平均を更新する関数
    void update_moving_average(float new_delta_x, float new_delta_y) {
        delta_x_buffer[buffer_index] = new_delta_x;
        delta_y_buffer[buffer_index] = new_delta_y;
        buffer_index = (buffer_index + 1) % MOVING_AVERAGE_SIZE;
    }

    // 移動平均を計算する関数
    void calculate_moving_average(float* avg_delta_x, float* avg_delta_y) {
        float sum_x = 0, sum_y = 0;
        for (int i = 0; i < MOVING_AVERAGE_SIZE; ++i) {
            sum_x += delta_x_buffer[i];
            sum_y += delta_y_buffer[i];
        }
        *avg_delta_x = sum_x / MOVING_AVERAGE_SIZE;
        *avg_delta_y = sum_y / MOVING_AVERAGE_SIZE;
    }

    typedef struct {
        int values[MA_WINDOW_SIZE];
        int index;
        int count;
    } MovingAverage;

    void initMovingAverage(MovingAverage* ma) {
        memset(ma, 0, sizeof(MovingAverage));
    }

    void updateMovingAverage(MovingAverage* ma, int newValue) {
        ma->values[ma->index] = newValue;
        ma->index = (ma->index + 1) % MA_WINDOW_SIZE;
        if (ma->count < MA_WINDOW_SIZE) {
            ma->count++;
        }
    }

    int calculateAverage(MovingAverage* ma) {
        int sum = 0;
        for (int i = 0; i < ma->count; i++) {
            sum += ma->values[i];
        }
        return ma->count > 0 ? sum / ma->count : 0;
    }
#endif



void process_modifier(uint8_t key, const char* type) {
    if (strcmp(type, "tap") == 0) {
        tap_code(key);
    } else if (strcmp(type, "hold") == 0) {
        register_code(key);
    } // "None" の場合は何もしない
}

#ifdef POINTING_DEVICE_ENABLE
    #ifdef CONSOLE_ENABLE
        #include <print.h>
        #include "wait.h"
        void keyboard_post_init_user(void) {
        debug_enable=true;
        debug_matrix=true;
        debug_mouse=true;
        }
    #endif

    #if JOYSTICK_MODE == 0
        #include "analog.h"
        #include "drivers/analog_joystick.h"

        // 各ジョイスティック軸のデータを初期化
        JoystickAxisData lxData = {0, 0, 0, false};
        JoystickAxisData lyData = {0, 0, 0, false};
        JoystickAxisData rxData = {0, 0, 0, false};
        JoystickAxisData ryData = {0, 0, 0, false};

        int16_t lxOrigin, lyOrigin, rxOrigin, ryOrigin;
        int8_t  JoystickAdjustmentFactor = 8;
        uint16_t current_time = 0;
        uint32_t initialScrollPeriod = 500; // 最初のスクロールタイミング
        uint32_t repeatScrollPeriod = 1900;   // 経過後のスクロール頻度の基準
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

        void process_joystick_axis_tap(JoystickAxisData *axisData, int16_t direction, uint8_t key_code_positive, uint8_t key_code_negative,
                               uint8_t modifier1, const char* modifier1_type,
                               uint8_t modifier2, const char* modifier2_type,
                               uint8_t modifier3, const char* modifier3_type) {
            if (abs(axisData->joyData) < 50) {
                axisData->initialScrollDone = false;
            } else {
                if (!axisData->initialScrollDone) {
                    if (direction > 0) {
                        process_modifier(modifier1, modifier1_type);
                        process_modifier(modifier2, modifier2_type);
                        process_modifier(modifier3, modifier3_type);
                        tap_code(key_code_positive);
                        if (strcmp(modifier3_type, "hold") == 0) {
                            unregister_code(modifier3);
                        }
                        if (strcmp(modifier2_type, "hold") == 0) {
                            unregister_code(modifier2);
                        }
                        if (strcmp(modifier1_type, "hold") == 0) {
                            unregister_code(modifier1);
                        }
                        axisData->lastTime1 = current_time;
                        axisData->initialScrollDone = true;
                    } else if (direction < 0) {
                        process_modifier(modifier1, modifier1_type);
                        process_modifier(modifier2, modifier2_type);
                        process_modifier(modifier3, modifier3_type);
                        tap_code(key_code_negative);
                        if (strcmp(modifier3_type, "hold") == 0) {
                            unregister_code(modifier3);
                        }
                        if (strcmp(modifier2_type, "hold") == 0) {
                            unregister_code(modifier2);
                        }
                        if (strcmp(modifier1_type, "hold") == 0) {
                            unregister_code(modifier1);
                        }
                        axisData->lastTime1 = current_time;
                        axisData->initialScrollDone = true;
                    }
                } else {
                    if (time_difference2(current_time, axisData->lastTime1) < initialScrollPeriod) {
                        axisData->lastTime2 = current_time;
                    } else {
                        if (time_difference2(current_time, axisData->lastTime2) > (repeatScrollPeriod / (direction * (direction / JoystickAdjustmentFactor)))) {
                            process_modifier(modifier1, modifier1_type);
                            process_modifier(modifier2, modifier2_type);
                            process_modifier(modifier3, modifier3_type);
                            tap_code((direction > 0) ? key_code_positive : key_code_negative);
                            if (strcmp(modifier3_type, "hold") == 0) {
                                unregister_code(modifier3);
                            }
                            if (strcmp(modifier2_type, "hold") == 0) {
                                unregister_code(modifier2);
                            }
                            if (strcmp(modifier1_type, "hold") == 0) {
                                unregister_code(modifier1);
                            }
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
            // process_joystick_axis_report(&lxData, data.lx, &mouse_report, &mouse_report.v, -1, 1);
            // process_joystick_axis_report(&lyData, data.ly, &mouse_report, &mouse_report.h, 1, -1);
            process_joystick_axis_tap(&lxData, data.lx, KC_VOLD, KC_VOLU, KC_NO, "None", KC_NO, "None", KC_NO, "None"); //ボリューム
            process_joystick_axis_tap(&lyData, data.ly, KC_RIGHT, KC_LEFT, KC_LCTL, "hold", KC_LGUI, "hold", KC_NO, "None"); //仮想デスクトップ
            process_joystick_axis_tap(&rxData, data.rx, KC_DOWN, KC_UP, KC_ESC, "tap", KC_LGUI, "hold", KC_NO, "None"); //画面移動
            process_joystick_axis_tap(&ryData, data.ry, KC_RIGHT, KC_LEFT, KC_ESC, "tap", KC_LGUI, "hold", KC_NO, "None"); //画面移動
            return mouse_report;
        }
    #endif

    void pointing_device_init_kb(void) {
        pmw33xx_init(0);         // index 1 is the fast device.
        pmw33xx_init(1);         // index 1 is the second device.a
        pmw33xx_set_cpi(0, 3000); // applies to first sensor
        pmw33xx_set_cpi(1, 3000); // applies to second sensor
        pointing_device_init_user();
    }

    // トラックボール部制御
    report_mouse_t pointing_device_task_kb(report_mouse_t mouse_report) {
        pmw33xx_report_t report0 = pmw33xx_read_burst(0);
        pmw33xx_report_t report1 = pmw33xx_read_burst(1);
        #define constrain_hid(amt) ((amt) < -127 ? -127 : ((amt) > 127 ? 127 : (amt)))
        tb_current_time = timer_read();

        // モーションフラグ
        if(motion_gesture_time < time_difference2(tb_current_time, tb_last_left_motion_time)){
            left_motion_detected = report0.delta_x != 0 || report0.delta_y != 0;
        }
        if(motion_gesture_time < time_difference2(tb_current_time, tb_last_right_motion_time)){
            right_motion_detected = report1.delta_x != 0 || report1.delta_y != 0;
        }
        bool dual_motion_detected = left_motion_detected && right_motion_detected;

        // トラックボールの方向計算
        left_ball_move_angle = 0;
        right_ball_move_angle = 0;
        if(abs(report0.delta_x) > ANGLE_THRESHOLD || abs(report0.delta_y) > ANGLE_THRESHOLD){
            left_ball_move_angle = atan2(-report0.delta_y, -report0.delta_x) * (180 / M_PI) + 180;
        }
        if(abs(report1.delta_x) > ANGLE_THRESHOLD || abs(report1.delta_y) > ANGLE_THRESHOLD){
            right_ball_move_angle = atan2(report1.delta_y, report1.delta_x) * (180 / M_PI) + 180;
        }

        if((left_ball_move_angle < TB_MOVE_ANGLE_THRESHOLD || left_ball_move_angle > 360 - TB_MOVE_ANGLE_THRESHOLD) && left_ball_move_angle != 0){
            tb_left_orient = 1; // "RIGHT"
        } else if(left_ball_move_angle < 90 + TB_MOVE_ANGLE_THRESHOLD && left_ball_move_angle > 90 - TB_MOVE_ANGLE_THRESHOLD){
            tb_left_orient = 2; // "UP"
        } else if(left_ball_move_angle < 180 + TB_MOVE_ANGLE_THRESHOLD && left_ball_move_angle > 180 - TB_MOVE_ANGLE_THRESHOLD){
            tb_left_orient = 3; // "LEFT"
        } else if(left_ball_move_angle < 270 + TB_MOVE_ANGLE_THRESHOLD && left_ball_move_angle > 270 - TB_MOVE_ANGLE_THRESHOLD){
            tb_left_orient = 4; // "DOWN"
        } else{
            tb_left_orient = 0;
        }

        if((right_ball_move_angle < TB_MOVE_ANGLE_THRESHOLD || right_ball_move_angle > 360 - TB_MOVE_ANGLE_THRESHOLD) && right_ball_move_angle != 0){
            tb_right_orient = 1; // "RIGHT"
        } else if(right_ball_move_angle < 90 + TB_MOVE_ANGLE_THRESHOLD && right_ball_move_angle > 90 - TB_MOVE_ANGLE_THRESHOLD){
            tb_right_orient = 2; // "UP"
        } else if(right_ball_move_angle < 180 + TB_MOVE_ANGLE_THRESHOLD && right_ball_move_angle > 180 - TB_MOVE_ANGLE_THRESHOLD){
            tb_right_orient = 3; // "LEFT"
        } else if(right_ball_move_angle < 270 + TB_MOVE_ANGLE_THRESHOLD && right_ball_move_angle > 270 - TB_MOVE_ANGLE_THRESHOLD){
            tb_right_orient = 4; // "DOWN"
        } else{
            tb_right_orient = 0;
        }

        #ifdef CONSOLE_ENABLE
            // if(left_motion_detected){
            //     uprintf("L %d\n ", left_ball_move_angle);
            //     uprintf("dm %d\n ", dual_motion_detected);
            //     uprintf("B3 %d\n ", button3_pressed);
            // }
            if(right_motion_detected){
                // uprintf("R %d\n ", right_ball_move_angle);
                // uprintf("dm %d\n ", dual_motion_detected);
                uprintf("WM %d\n, B3 %d\n ", dual_motion_detected, button3_pressed);
            }
            // if(dual_motion_detected){
            //     uprintf("W %d\n ", left_ball_move_angle - right_ball_move_angle);
            //     uprintf("dm %d\n ", dual_motion_detected);
            //     uprintf("B3 %d\n ", button3_pressed);
            // }
        #endif


        // left
        if (left_motion_detected) {
            #if TRACKBALL_MODE == 0
                tb_last_left_motion_time = timer_read();
                if (tb_left_orient == 1 || tb_left_orient == 3) {
                    accumulated_h += (float)(report0.delta_x);
                    float accumulated_h2 = accumulated_h / 1000 * X_SC_SCALE_FACTOR;
                    if (fabs(accumulated_h2) >= 1.0f) {
                        mouse_report.h = (accumulated_h2 > 0) ? 1 : -1;
                        accumulated_h = 0; // Reset
                    }
                }
                if (tb_left_orient == 2 || tb_left_orient == 4) {
                    accumulated_v += (float)(report0.delta_y);
                    float accumulated_v2 = accumulated_v / 800 * Y_SC_SCALE_FACTOR;
                    if (fabs(accumulated_v2) >= 1.0f) {
                        mouse_report.v = (accumulated_v2 > 0) ? 1 : -1;
                        accumulated_v = 0; // Reset
                    }
                }
            #elif TRACKBALL_MODE == 1
                if(!shift_pressed){
                    if (!dual_motion_detected) {
                    // if(time_difference2(tb_current_time, left_init_time) > TB_RETURN_TIME){
                        register_code(KC_LSFT);
                        shift_pressed = true;
                        wait_ms(1);
                        register_code(KC_BTN3);
                        button3_pressed = true;
                    // }
                    }
                }
                mouse_report.x = constrain_hid(mouse_report.x + report0.delta_x / XSCALE_FACTOR);
                mouse_report.y = constrain_hid(mouse_report.y + -report0.delta_y / YSCALE_FACTOR);

            #endif
        }

        // right
        if (right_motion_detected) {
            tb_last_right_motion_time = timer_read();
            mouse_report.x = constrain_hid(mouse_report.x + -report1.delta_x / XSCALE_FACTOR);
            mouse_report.y = constrain_hid(mouse_report.y + report1.delta_y / YSCALE_FACTOR);
        }

        // Dual
        if (dual_motion_detected) {
            // 通常モード
            #if TRACKBALL_MODE == 0
                if(tb_left_orient - tb_right_orient == 0){
                    if(mouse_report.h == 1){
                        tap_code(KC_RIGHT);
                    } else if(mouse_report.v == 1){
                        tap_code(KC_UP);
                    } else if(mouse_report.h == -1){
                        tap_code(KC_LEFT);
                    } else if(mouse_report.v == -1){
                        tap_code(KC_DOWN);
                    }
                }
                mouse_report.v = 0;
                mouse_report.h = 0;
                mouse_report.x = 0;
                mouse_report.y = 0;

            // CADモード
            #elif TRACKBALL_MODE == 1
                mouse_report.x = mouse_report.x / 2;
                mouse_report.y = mouse_report.y / 2;
                mouse_report.v = 0;
                mouse_report.h = 0;
                if ((tb_right_orient == 1 && tb_left_orient == 3) || (tb_right_orient == 3 && tb_left_orient == 1)) {
                        unregister_code(KC_LSFT);
                        shift_pressed = false;
                        unregister_code(KC_BTN3);
                        wait_ms(1);
                        mouse_report.x = 0;
                        mouse_report.y = 0;
                        mouse_report.v = (report1.delta_x > 0) ? -1 : 1;
                }else if(tb_right_orient != 0){
                    if(tb_right_orient == tb_left_orient){
                        if(!button3_pressed){
                            if(time_difference2(tb_current_time, dual_init_time) > TB_RETURN_TIME){
                                unregister_code(KC_LSFT);
                                shift_pressed = false;
                                wait_ms(1);
                                custom_register_mouse(KC_MS_BTN3, true);
                                button3_pressed = true;
                                dual_motion_time = timer_read();
                            }
                        }
                    }
                }
            #endif
        }
        return pointing_device_task_user(mouse_report);
    }

    void housekeeping_task_kb(void) {
        if (!left_motion_detected) {
            if (shift_pressed) {
                if (time_difference2(tb_current_time, left_init_time) > INACTIVITY_TIMEOUT) {
                    left_init_time = timer_read();
                    unregister_code(KC_LSFT);
                    shift_pressed = false;
                }
            }
        }

        if (!dual_motion_detected && !left_motion_detected) {
            if (time_difference2(tb_current_time, dual_init_time) > INACTIVITY_TIMEOUT) {
                if(time_difference2(tb_current_time, dual_motion_time) > TB_MOUSE_BOTTON_PUSH_TIME){
                    dual_init_time = timer_read();
                    // if (ctrl_pressed) {
                    //     unregister_code(KC_LCTL);
                    //     ctrl_pressed = false;
                    // }
                    if (button3_pressed) {
                            custom_register_mouse(KC_MS_BTN3, false);
                            button3_pressed = false;
                    }
                }
            }
        }
        housekeeping_task_user();
    }


#endif




