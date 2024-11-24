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
#include "analog.h"
#include "drivers/analog_joystick.h"

#ifdef CONSOLE_ENABLE
    int8_t debugPMW3360x, debugPMW3360y, debugPMW3360xy = 0;
#endif

#define ANGLE_THRESHOLD 1
#define TB_MOVE_ANGLE_THRESHOLD 45
#define XSCALE_FACTOR 10
#define YSCALE_FACTOR 10
#define X_SC_SCALE_FACTOR 5.0f
#define Y_SC_SCALE_FACTOR 5.0f
#define MA_WINDOW_SIZE 10 //移動平均のサイズ
#define MOVING_AVERAGE_SIZE 10 // 移動平均のサンプルサイズ
#define INACTIVITY_TIMEOUT 100 // msec無操作でリセット
#define TB_RETURN_TIME 50 // msec無操作でリセット
#define TB_MOUSE_BOTTON_PUSH_TIME 100 // msec以上経過しないとボタンを押し直さない
#define constrain_hid(amt) ((amt) < -127 ? -127 : ((amt) > 127 ? 127 : (amt)))

// // keymap.cの仮想ボタン配列を外部から利用できるようにする
extern uint16_t virtual_keys[4][MATRIX_ROWS / 2][MATRIX_COLS * 2 - 4];
uint16_t prev_virtual_keys[4][MATRIX_ROWS / 2][MATRIX_COLS * 2 - 4];

bool is_keymap_changed(void) {
    // キーマップが変更されたかどうかを確認する
    for (int row = 0; row < MATRIX_ROWS / 2; row++) {  // 行のループ
        for (int j = 0; j < MATRIX_COLS * 2 - 4; j++) {  // 列のループ
            if (prev_virtual_keys[0][row][j] != virtual_keys[0][row][j] ||
                prev_virtual_keys[1][row][j] != virtual_keys[1][row][j] ||
                prev_virtual_keys[2][row][j] != virtual_keys[2][row][j] ||
                prev_virtual_keys[3][row][j] != virtual_keys[3][row][j]) {
                return true; // 変更があった
            }
        }
    }
    return false; // 変更なし
}

uint8_t get_wait_time(uint16_t keycode) {
    switch (keycode) {
        case KC_0:
            return 1;
        case KC_1:
            return 2;
        case KC_2:
            return 3;
        case KC_3:
            return 5;
        case KC_4:
            return 10;
        case KC_5:
            return 20;
        case KC_6:
            return 30;
        case KC_7:
            return 50;
        case KC_8:
            return 100;
        case KC_9:
            return 200;
        default:
            return 1;
    }
}

const char* get_modifier_type(uint16_t keycode) {
    switch (keycode) {
        case KC_0:
            return "hold";
        case KC_1:
            return "tap";
        default:
            return "None";
    }
}

int calculate_cpi(uint8_t keycode) {
    return 1000 + (keycode - KC_1) * 500;  // KC_0 -> 1000, KC_1 -> 1500, ..., KC_9 -> 5000
}

uint16_t tb_left_cpi, tb_right_cpi;

bool tb_l_pressed[MATRIX_COLS * 2 - 4];
bool tb_r_pressed[MATRIX_COLS * 2 - 4];
bool tb_d_pressed[MATRIX_COLS * 2 - 4];
bool js_l_pressed[MATRIX_COLS * 2 - 4];
bool js_r_pressed[MATRIX_COLS * 2 - 4];

float tb_speed_fact;

uint16_t tb_move_type;
uint8_t current_layer = 0;

// bool joystick_keymap_flag, tb_keymap_flag = false;

// 下記はdirectピンを倍マトリクスかしているため使えない
// virtual_keys[*][*][0]S
// virtual_keys[*][*][1]
// virtual_keys[*][*][10]
// virtual_keys[*][*][11]

void get_tb_virtual_keys(void) {
    tb_left_cpi             = calculate_cpi(virtual_keys[current_layer][1][0]);
    tb_right_cpi            = calculate_cpi(virtual_keys[current_layer][3][0]);
}

// bool ctrl_pressed = false;
bool left_motion_detected, right_motion_detected, dual_motion_detected = false;
int8_t tb_left_orient, tb_right_orient = 0;
int8_t joy_left_orient, joy_right_orient = 0;
uint16_t tb_last_left_motion_time,tb_last_right_motion_time = 0;
uint16_t tb_current_time = 0;
uint16_t init_time = 0;
uint16_t dual_init_time = 0;
uint16_t left_init_time = 0;
uint16_t right_init_time = 0;
uint16_t dual_motion_time = 0;
uint16_t motion_gesture_time = 20;
int16_t left_ball_move_angle = 0;
int16_t right_ball_move_angle = 0;
bool layer_changed_flag = false;


uint16_t time_difference2(uint16_t t1, uint16_t t2){
    return (t1 - t2);
}

report_add tb_add_report;
// 移動平均を計算
float avg_delta_x = 0.0f;
float avg_delta_y = 0.0f;

// スクロール

static float accumulated_h = 0.0f;
static float accumulated_v = 0.0f;
static float accumulated_h2 = 0.0f;
static float accumulated_v2 = 0.0f;


// 移動平均のためのバッファとインデックス
float delta_x_buffer[MOVING_AVERAGE_SIZE] = {0};
float delta_y_buffer[MOVING_AVERAGE_SIZE] = {0};
int buffer_index = 0;
int tb_scaled;
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

void reset_moving_average(void) {
    for (int i = 0; i < MOVING_AVERAGE_SIZE; ++i) {
        delta_x_buffer[i] = 0;
        delta_y_buffer[i] = 0;
    }
    avg_delta_x = 0;
    avg_delta_y = 0;
    buffer_index = 0;
}

void get_report_add(report_add *tb_add_report, float delta_x, float delta_y, int orientation, float speed_adjust, uint16_t cpi) {
    float x = delta_x;
    float y = delta_y;
    uprintf("x: %d\n" , (int)x);

    int sign_x = (x > 0) - (x < 0);
    int sign_y = (y > 0) - (y < 0);

    x = pow(fabs(x), speed_adjust)/(pow(cpi / 3, speed_adjust))*cpi / 3 * sign_x;
    y = pow(fabs(y), speed_adjust)/(pow(cpi / 3, speed_adjust))*cpi / 3 * sign_y;

    // 水平
    if (orientation == 1 || orientation == 3) {
        accumulated_h += x;
        accumulated_h2 = accumulated_h / 500 * X_SC_SCALE_FACTOR;
        if (fabs(accumulated_h2) >= 1.0f) {
            tb_add_report->lr = (accumulated_h2 > 0) ? 1 : -1;
            accumulated_h = 0;
        }
    }
    // 垂直
    if (orientation == 2 || orientation == 4) {
        accumulated_v += y;
        accumulated_v2 = accumulated_v / 500 * Y_SC_SCALE_FACTOR;
        if (fabs(accumulated_v2) >= 1.0f) {
            tb_add_report->ud = (accumulated_v2 > 0) ? 1 : -1;
            accumulated_v = 0;
        }
    }
    // uprintf("tb_scaled: %d\n" , tb_scaled);
}

void scroll_report(report_mouse_t *mouse_report, report_add *tb_add_report, float delta_x, float delta_y, int orientation) {
    mouse_report->h = tb_add_report->lr;
    mouse_report->v = tb_add_report->ud;
}

void cursor_report(report_mouse_t *mouse_report, float delta_x, float delta_y, float speed_adjust, uint16_t cpi) {

    float x = delta_x;
    float y = delta_y;

    int sign_x = (x > 0) - (x < 0);
    int sign_y = (y > 0) - (y < 0);

    x = pow(fabs(x), speed_adjust) / (pow(cpi / 3, speed_adjust)) * cpi / 3 * sign_x;
    y = pow(fabs(y), speed_adjust) / (pow(cpi / 3, speed_adjust)) * cpi / 3 * sign_y;

    x = x / XSCALE_FACTOR;
    y = y / YSCALE_FACTOR;

    mouse_report->x = constrain_hid(mouse_report->x + (int8_t)roundf(x));
    mouse_report->y = constrain_hid(mouse_report->y + (int8_t)roundf(y));

    // mouse_report->x = constrain_hid(mouse_report->x + (-delta_x / XSCALE_FACTOR));
    // mouse_report->y = constrain_hid(mouse_report->y + (delta_y / YSCALE_FACTOR));
}

void arrow_report(report_add *tb_add_report, float delta_x, float delta_y, int orientation) {
    if(tb_add_report->lr == 1){
        tap_code(KC_RIGHT);
    }
    if(tb_add_report->ud == 1){
        tap_code(KC_UP);
    }
    if(tb_add_report->lr == -1){
        tap_code(KC_LEFT);
    }
    if(tb_add_report->ud == -1){
        tap_code(KC_DOWN);
    }
}

void tap_report(int orient, uint8_t press_key1, uint16_t press_key2, uint16_t press_key3, uint16_t press_key4) {
    // for (int i = 0; i < tb_scaled; i++){
    if(orient == 1){
        tap_code(press_key1);
    }else if(orient == 2){
        tap_code(press_key2);
    }else if(orient == 3){
        tap_code(press_key3);
    }else if(orient == 4){
        tap_code(press_key4);
    }
    // }
}

void process_modifier(uint8_t key, const char* type) {
    if (strcmp(type, "tap") == 0) {
        tap_code(key);
    } else if (strcmp(type, "hold") == 0) {
        register_code(key);
    } // "None" の場合は何もしない
}

bool process_trackball_modifier(uint16_t press_key, const char* press_type, bool pressed) {
    if (press_key != KC_NO) {
        if (strcmp(press_type, "tap") == 0) {
            tap_code(press_key);
        } else if (strcmp(press_type, "hold") == 0) {
            register_code(press_key);
            pressed = true;
        } // "None" の場合は何もしない
    }
    return pressed;
}

void process_trackball_motion(bool *pressed, uint8_t row, uint16_t wait_time, uint8_t tb_mode, report_mouse_t *mouse_report, int16_t delta_x, int16_t delta_y, int tb_orient, uint8_t current_layer, uint8_t press_key1, uint16_t press_key2, uint16_t press_key3, uint16_t press_key4, float tb_speed_adjust, report_add *tb_add_report, uint16_t cpi) {
    uint8_t col_strat = 0, col_end = 0;
    tb_scaled = sqrt(accumulated_h2 * accumulated_h2 + accumulated_v2 * accumulated_v2);

    // トラックボール方向に基づく列範囲設定
    if (tb_add_report->lr != 0 || tb_add_report->ud != 0) {
        switch (tb_orient) {
            case 1: col_strat = 4; col_end = 6; break; // right
            case 2: col_strat = 10; col_end = 12; break; // up
            case 3: col_strat = 7; col_end = 9; break; // left
            case 4: col_strat = 13; col_end = 15; break; // down
            default: return;
        }

        switch (tb_mode) {
            case KC_0:
                for (int i = col_strat; i <= col_end; i++) {
                    if (!pressed[i]) {
                        pressed[i] = process_trackball_modifier(
                            virtual_keys[current_layer][row][i],
                            get_modifier_type(virtual_keys[current_layer][row + 1][i]),
                            pressed[i]
                        );
                        wait_us(1000);
                    }
                }
                cursor_report(mouse_report, delta_x, -delta_y, tb_speed_adjust, cpi);
                break;

            case KC_1:
                for (int i = 0; i < tb_scaled; i++){
                    for (int i = col_strat; i <= col_end; i++) {
                        if (!pressed[i]) {
                            pressed[i] = process_trackball_modifier(
                                virtual_keys[current_layer][row][i],
                                get_modifier_type(virtual_keys[current_layer][row + 1][i]),
                                pressed[i]
                            );
                        wait_ms(1);
                        }
                    }
                    tap_report(tb_orient, press_key1, press_key2, press_key3, press_key4);
                }
                tb_add_report->lr = 0;
                tb_add_report->ud = 0;
                break;
            default:
                break;
        }
    }
}

bool handle_motion_register(bool pressed, uint16_t mod_keycode) {
    if (!pressed){
        register_code(mod_keycode);    // モディファイアキーを押下
        pressed = true;      // モディファイアキーが押下されたことを示す
    }
    return pressed;
}

bool handle_motion_unregister(bool pressed, uint16_t mod_keycode) {
    if (pressed) {
        unregister_code(mod_keycode);    // モディファイアキーを解除
        pressed = false;      // モディファイアキーが解除されたことを示す
    }
    return pressed;
}

// joystick
uint8_t joy_orient_threshold = 30;
uint8_t joy_wait_time;
float joy_l_speed_fact = 1.0f;
float joy_l_speed_fact2 = 1.0f;
float joy_r_speed_fact = 1.0f;
float joy_r_speed_fact2 = 1.0f;
float get_speed_adjust(uint16_t keycode) {
    switch (keycode) {
        case KC_1:
            return 0.7;
        case KC_2:
            return 0.8;
        case KC_3:
            return 0.9;
        case KC_4:
            return 0.95;
        case KC_5:
            return 1;
        case KC_6:
            return 1.1;
        case KC_7:
            return 1.2;
        case KC_8:
            return 1.4;
        case KC_9:
            return 1.6;
        case KC_0:
            return 2.0;
        default:
            return 1;
    }
}
int16_t lxJoyData, lyJoyData, rxJoyData, ryJoyData;
int16_t lxOrigin, lyOrigin, rxOrigin, ryOrigin;
int8_t  JoystickAdjustmentFactor = 8;
uint16_t joy_current_time_L = 0;
uint16_t joy_current_time_R = 0;
// uint16_t joy_time_count_L = 0;
// uint16_t joy_time_count_R = 0;
uint16_t joy_initial_time_count_L = 0; // 最初のスクロールタイミング
uint16_t joy_repeat_time_count_L = 0; // 経過後のスクロール頻度の基準
uint16_t joy_initial_time_count_R = 0; // 最初のスクロールタイミング
uint16_t joy_repeat_time_count_R = 0; // 経過後のスクロール頻度の基準
uint16_t joy_initial_time_period = 250; // 最初のスクロールタイミング
uint16_t joy_repeat_time_period = 200;   // 経過後のスクロール頻度の基準
bool joyInitialFlagL = false;
bool joyInitialFlagR = false;
float joyPressFactL, joyPressFactR;
bool joyCursorMoveL = false;
bool joyCursorMoveR = false;
bool joyCursorMoveFlagL = false;
bool joyCursorMoveFlagR = false;

void process_joystick_press(bool *pressed, uint8_t row, uint8_t wait_time, uint8_t orient, uint8_t current_layer, bool joyCursorMove){
    uint8_t col_strat = 0, col_end = 0 , j = 0 , k = 0;
    //右下が＋
    switch (orient) {
        case 1: //right
            col_strat = 4;
            col_end = 6;
            j = 0;
            k = 2;
            break;
        case 2: //up
            col_strat = 10;
            col_end = 12;
            j = 1;
            k = 2;
            break;
        case 3: //left
            col_strat = 7;
            col_end = 9;
            j = 0;
            k = 3;
            break;
        case 4: //down
            col_strat = 13;
            col_end = 15;
            j = 1;
            k = 3;
            break;
        default:
            return;
    }

    for (int i = col_strat; i <= col_end; i++) {
        if (!pressed[i]) {
            pressed[i] = process_trackball_modifier(virtual_keys[current_layer][row][i], get_modifier_type(virtual_keys[current_layer][row+1][i]), pressed[i]);
        }
        // if (virtual_keys[current_layer][row][i] != 0) {
            wait_ms(1);
        // }
    }
    if (!joyCursorMove) {
        tap_code(virtual_keys[current_layer][row+j][k]);
    }
}






// トラックパッド機能の有効化
// #include "i2c_master.h"
// #include "drivers/azoteq_iqs5xx.h"  // Azoteq関連のすべての機能
// // #include "drivers/azoteq_iqs5xx.c"  // Azoteq関連のすべての機能
// void azoteq_process_scroll(const azoteq_iqs5xx_base_data_t *base_data) {
//     static float accumulated_h = 0.0f;
//     static float accumulated_v = 0.0f;

//     float delta_x = AZOTEQ_IQS5XX_COMBINE_H_L_BYTES(base_data->x.h, base_data->x.l);
//     float delta_y = AZOTEQ_IQS5XX_COMBINE_H_L_BYTES(base_data->y.h, base_data->y.l);

//     accumulated_h += delta_x;
//     accumulated_v += delta_y;

//     if (fabs(accumulated_h) >= 32.0f) { // Azoteqの閾値
//         pointing_device_set_report_h((accumulated_h > 0) ? 1 : -1);
//         accumulated_h = 0;
//     }

//     if (fabs(accumulated_v) >= 32.0f) { // Azoteqの閾値
//         pointing_device_set_report_v((accumulated_v > 0) ? 1 : -1);
//         accumulated_v = 0;
//     }
// }
// void pmw3360_to_azoteq_scroll(pmw33xx_report_t report0, pmw33xx_report_t report1) {
//     azoteq_iqs5xx_base_data_t base_data;

//     // PMW3360のデータをAzoteq形式に詰め替える
//     base_data.x.h = (report0.delta_x + report1.delta_x) >> 8;
//     base_data.x.l = (report0.delta_x + report1.delta_x) & 0xFF;
//     base_data.y.h = (report0.delta_y + report1.delta_y) >> 8;
//     base_data.y.l = (report0.delta_y + report1.delta_y) & 0xFF;

//     // スクロールフラグを有効化（必要に応じてカスタマイズ）
//     base_data.gesture_events_1.scroll = true;

//     // Azoteqのスクロール処理を呼び出し
//     azoteq_process_scroll(&base_data);
// }











#ifdef POINTING_DEVICE_ENABLE

    void pointing_device_driver_init(void) {
        analog_joystick_init();
    }

    void pointing_device_init_kb(void) {
        pmw33xx_init(0);         // index 1 is the fast device.
        pmw33xx_init(1);         // index 1 is the second device.a
        pmw33xx_set_cpi(0, tb_left_cpi); // applies to first sensor
        pmw33xx_set_cpi(1, tb_right_cpi); // applies to second sensor
        pointing_device_init_user();
        for (int i = 0; i < MATRIX_COLS * 2 - 4; i++) {
            tb_l_pressed[i] = false;
            tb_r_pressed[i] = false;
            tb_d_pressed[i] = false;
            js_l_pressed[i] = false;
            js_r_pressed[i] = false;
        }
    }

    report_mouse_t pointing_device_task_kb(report_mouse_t mouse_report) {
        // トラックボール部制御
        if(is_keymap_changed()) {
            get_tb_virtual_keys();
            pmw33xx_set_cpi(0, tb_left_cpi); // applies to first sensor
            pmw33xx_set_cpi(1, tb_right_cpi); // applies to second sensor
        }
        if(layer_changed_flag) {
            get_tb_virtual_keys();
            pmw33xx_set_cpi(0, tb_left_cpi); // applies to first sensor
            pmw33xx_set_cpi(1, tb_right_cpi); // applies to second sensor
            layer_changed_flag = false;

        }

        pmw33xx_report_t report0 = pmw33xx_read_burst(0);
        pmw33xx_report_t report1 = pmw33xx_read_burst(1);



        // // トラックパッド機能の有効化
        // pmw3360_to_azoteq_scroll(report0, report1);







        tb_current_time = timer_read();

        // モーションフラグ
        if(motion_gesture_time < time_difference2(tb_current_time, tb_last_left_motion_time)){
            left_motion_detected = report0.delta_x != 0 || report0.delta_y != 0;
            tb_last_left_motion_time = timer_read();
        }
        if(motion_gesture_time < time_difference2(tb_current_time, tb_last_right_motion_time)){
            right_motion_detected = report1.delta_x != 0 || report1.delta_y != 0;
            tb_last_right_motion_time = timer_read();
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

        current_layer = get_highest_layer(layer_state);
        // uprintf("v: %d\n" , virtual_keys[current_layer][2][2]);
        // uprintf("r: %d\n" , KC_RIGHT);

        // if (dual_motion_detected) {
        //     tb_speed_fact = get_speed_adjust(virtual_keys[current_layer][4][1]);
            // if (virtual_keys[current_layer][5][0] == KC_0) {
        //         get_report_add(&tb_add_report, report0.delta_x, report0.delta_y, tb_left_orient);
        //         process_trackball_motion_press(tb_d_pressed, 4, tb_speed_fact, tb_right_orient, current_layer, &tb_add_report);
        //         process_trackball_motion_move(dual_motion_detected, virtual_keys[current_layer][4][0], &tb_add_report, &mouse_report, report0.delta_x, report0.delta_y, tb_left_orient, virtual_keys[current_layer][4][2], virtual_keys[current_layer][5][2], virtual_keys[current_layer][4][3], virtual_keys[current_layer][5][3], tb_speed_fact);
        //     } else {
        //         get_report_add(&tb_add_report, -report1.delta_x, -report1.delta_y, tb_right_orient);
        //         process_trackball_motion_press(tb_d_pressed, 4, tb_speed_fact, tb_right_orient, current_layer, &tb_add_report);
        //         process_trackball_motion_move(dual_motion_detected, virtual_keys[current_layer][4][0], &tb_add_report, &mouse_report, -report1.delta_x, -report1.delta_y, tb_right_orient, virtual_keys[current_layer][4][2], virtual_keys[current_layer][5][2], virtual_keys[current_layer][4][3], virtual_keys[current_layer][5][3], tb_speed_fact);
        //     }

        // } else if (left_motion_detected) {
        //     tb_speed_fact = get_speed_adjust(virtual_keys[current_layer][0][1]);
        //     get_report_add(&tb_add_report, report0.delta_x, report0.delta_y, tb_left_orient);
        //     process_trackball_motion_press(tb_l_pressed, 0, tb_speed_fact, tb_left_orient, current_layer, &tb_add_report);
        //     process_trackball_motion_move(left_motion_detected, virtual_keys[current_layer][0][0], &tb_add_report, &mouse_report, report0.delta_x, report0.delta_y, tb_left_orient, virtual_keys[current_layer][0][2], virtual_keys[current_layer][1][2], virtual_keys[current_layer][0][3], virtual_keys[current_layer][1][3], tb_speed_fact);

        // } else if (right_motion_detected) {
        //     tb_speed_fact = get_speed_adjust(virtual_keys[current_layer][2][1]);
        //     get_report_add(&tb_add_report, -report1.delta_x, -report1.delta_y, tb_right_orient);
        //     process_trackball_motion_press(tb_r_pressed, 2, tb_speed_fact, tb_right_orient, current_layer, &tb_add_report);
        //     process_trackball_motion_move(right_motion_detected, virtual_keys[current_layer][2][0], &tb_add_report, &mouse_report, -report1.delta_x, -report1.delta_y, tb_right_orient, virtual_keys[current_layer][2][2], virtual_keys[current_layer][3][2], virtual_keys[current_layer][2][3], virtual_keys[current_layer][3][3], tb_speed_fact);
        // }

        if (dual_motion_detected) {
            tb_speed_fact = get_speed_adjust(virtual_keys[current_layer][4][1]);

            if (virtual_keys[current_layer][5][0] == KC_0) {
                get_report_add(&tb_add_report, report0.delta_x, report0.delta_y, tb_left_orient, tb_speed_fact, tb_left_cpi);
                process_trackball_motion(
                    tb_d_pressed, 4, tb_speed_fact, virtual_keys[current_layer][4][0], &mouse_report,
                    report0.delta_x, report0.delta_y, tb_left_orient, current_layer,
                    virtual_keys[current_layer][4][2], virtual_keys[current_layer][5][2],
                    virtual_keys[current_layer][4][3], virtual_keys[current_layer][5][3],
                    tb_speed_fact, &tb_add_report, tb_left_cpi
                );
            } else {
                get_report_add(&tb_add_report, -report1.delta_x, -report1.delta_y, tb_right_orient, tb_speed_fact, tb_right_cpi);
                process_trackball_motion(
                    tb_d_pressed, 4, tb_speed_fact, virtual_keys[current_layer][4][0], &mouse_report,
                    -report1.delta_x, -report1.delta_y, tb_right_orient, current_layer,
                    virtual_keys[current_layer][4][2], virtual_keys[current_layer][5][2],
                    virtual_keys[current_layer][4][3], virtual_keys[current_layer][5][3],
                    tb_speed_fact, &tb_add_report, tb_right_cpi
                );
            }
        } else if (left_motion_detected) {
            tb_speed_fact = get_speed_adjust(virtual_keys[current_layer][0][1]);
            get_report_add(&tb_add_report, report0.delta_x, report0.delta_y, tb_left_orient, tb_speed_fact, tb_left_cpi);
            process_trackball_motion(
                tb_l_pressed, 0, tb_speed_fact, virtual_keys[current_layer][0][0], &mouse_report,
                report0.delta_x, report0.delta_y, tb_left_orient, current_layer,
                virtual_keys[current_layer][0][2], virtual_keys[current_layer][1][2],
                virtual_keys[current_layer][0][3], virtual_keys[current_layer][1][3],
                tb_speed_fact, &tb_add_report, tb_left_cpi
            );
        } else if (right_motion_detected) {
            tb_speed_fact = get_speed_adjust(virtual_keys[current_layer][2][1]);
            get_report_add(&tb_add_report, -report1.delta_x, -report1.delta_y, tb_right_orient, tb_speed_fact, tb_right_cpi);
            process_trackball_motion(
                tb_r_pressed, 2, tb_speed_fact, virtual_keys[current_layer][2][0], &mouse_report,
                -report1.delta_x, -report1.delta_y, tb_right_orient, current_layer,
                virtual_keys[current_layer][2][2], virtual_keys[current_layer][3][2],
                virtual_keys[current_layer][2][3], virtual_keys[current_layer][3][3],
                tb_speed_fact, &tb_add_report, tb_right_cpi
            );
        }




        // joystick
        // report_analog_joystick_t data = analog_joystick_read();



        joy_l_speed_fact = get_speed_adjust(virtual_keys[current_layer][7][0]);
        joy_l_speed_fact2 = get_speed_adjust(virtual_keys[current_layer][6][1]);
        joy_r_speed_fact = get_speed_adjust(virtual_keys[current_layer][9][0]);
        joy_r_speed_fact2 = get_speed_adjust(virtual_keys[current_layer][8][1]);

        lxJoyData = (analogReadPin(ANALOG_JOYSTICK_LX_AXIS_PIN) - lxOrigin) / 2 * joy_l_speed_fact;
        lyJoyData = (analogReadPin(ANALOG_JOYSTICK_LY_AXIS_PIN) - lyOrigin) / 2 * joy_l_speed_fact;
        rxJoyData = (analogReadPin(ANALOG_JOYSTICK_RX_AXIS_PIN) - rxOrigin) / 2 * joy_r_speed_fact;
        ryJoyData = (analogReadPin(ANALOG_JOYSTICK_RY_AXIS_PIN) - ryOrigin) / 2 * joy_r_speed_fact;
        joyPressFactL = (abs(lxJoyData) + abs(lyJoyData)) / 20;
        joyPressFactR = (abs(rxJoyData) + abs(ryJoyData)) / 20;

        joy_wait_time = get_wait_time(virtual_keys[current_layer][6][1]);
        if (lxJoyData * lxJoyData > lyJoyData * lyJoyData) {
            if (lxJoyData > joy_orient_threshold) {
                joy_left_orient = 1;
            }else if (lxJoyData < -joy_orient_threshold) {
                joy_left_orient = 3;
            }else {
                joy_left_orient = 0;
            }

        }else {
            if (lyJoyData > joy_orient_threshold) {
                joy_left_orient = 4;
            }else if (lyJoyData < -joy_orient_threshold) {
                joy_left_orient = 2;
            }else {
                joy_left_orient = 0;
            }
        }

        switch (virtual_keys[current_layer][6][0]) {
            case KC_0:
                joyCursorMoveL = true;
                if (lxJoyData > joy_orient_threshold || lyJoyData > joy_orient_threshold || lxJoyData < -joy_orient_threshold || lyJoyData < -joy_orient_threshold) {
                    joyCursorMoveFlagL = true;
                    process_joystick_press(js_l_pressed, 6, joy_wait_time, joy_left_orient, current_layer, joyCursorMoveL);
                    cursor_report(&mouse_report, lxJoyData, lyJoyData, joy_l_speed_fact2, 128*3);
                }else {
                    joyCursorMoveFlagL = false;
                }

                break;

            case KC_1:
                joyCursorMoveL = false;
                if (abs(lxJoyData) > joy_orient_threshold || abs(lyJoyData) > joy_orient_threshold) {
                    joy_current_time_L = timer_read();
                    if (!joyInitialFlagL) {
                        joy_initial_time_count_L = timer_read();
                        process_joystick_press(js_l_pressed, 6, joy_wait_time, joy_left_orient, current_layer, joyCursorMoveL);
                        joyInitialFlagL = true;

                    }else {
                        if (time_difference2(joy_current_time_L, joy_initial_time_count_L) > ((joy_initial_time_period + joy_repeat_time_period) / joy_l_speed_fact2)) {
                            // 下にジョイスティックの倒し量により速度を変える変数をいれる
                            if (time_difference2(joy_current_time_L, joy_repeat_time_count_L) > (joy_repeat_time_period / joy_l_speed_fact / joyPressFactL)) {
                                joy_repeat_time_count_L = timer_read();
                                process_joystick_press(js_l_pressed, 6, joy_wait_time, joy_left_orient, current_layer, joyCursorMoveL);
                            }
                        }
                    }

                } else {
                     joyInitialFlagL = false;
                }

                break;

            default:
                break;
        }


        joy_wait_time = get_wait_time(virtual_keys[current_layer][8][1]);
        if (rxJoyData * rxJoyData > ryJoyData * ryJoyData) {
            if (rxJoyData > joy_orient_threshold) {
                joy_right_orient = 1;
            }else if (rxJoyData < -joy_orient_threshold) {
                joy_right_orient = 3;
            }else {
                joy_right_orient = 0;
            }
        }else {
            if (ryJoyData > joy_orient_threshold) {
                joy_right_orient = 4;
            }else if (ryJoyData < -joy_orient_threshold) {
                joy_right_orient = 2;
            }else {
                joy_right_orient = 0;
            }
        }

        switch (virtual_keys[current_layer][8][0]) {
            case KC_0:
                joyCursorMoveR = true;
                if (rxJoyData > joy_orient_threshold || ryJoyData > joy_orient_threshold || rxJoyData < -joy_orient_threshold || ryJoyData < -joy_orient_threshold) {
                    joyCursorMoveFlagR = true;
                    process_joystick_press(js_r_pressed, 8, joy_wait_time, joy_right_orient, current_layer, joyCursorMoveR);
                    cursor_report(&mouse_report, rxJoyData, ryJoyData, joy_r_speed_fact2, 128*3);
                }else {
                    joyCursorMoveFlagR = false;
                }

                break;

            case KC_1:
                joyCursorMoveR = false;
                if (abs(rxJoyData) > joy_orient_threshold || abs(ryJoyData) > joy_orient_threshold) {
                    joy_current_time_R = timer_read();
                    if (!joyInitialFlagR) {
                        joy_initial_time_count_R = timer_read();
                        process_joystick_press(js_r_pressed, 8, joy_wait_time, joy_right_orient, current_layer, joyCursorMoveR);
                        joyInitialFlagR = true;

                    }else {
                        if (time_difference2(joy_current_time_R, joy_initial_time_count_R) > ((joy_initial_time_period + joy_repeat_time_period) / joy_r_speed_fact2)) {
                            // 下にジョイスティックの倒し量により速度を変える変数をいれる
                            if (time_difference2(joy_current_time_R, joy_repeat_time_count_R) > (joy_repeat_time_period / joy_r_speed_fact / joyPressFactR)) {
                                joy_repeat_time_count_R = timer_read();
                                process_joystick_press(js_r_pressed, 8, joy_wait_time, joy_right_orient, current_layer, joyCursorMoveR);
                            }
                        }
                    }

                } else {
                     joyInitialFlagR = false;
                }

                break;

            default:
                break;
        }

        // uprintf("lx,ly,rx,ry: %d %d %d %d\n" , lxJoyData, lyJoyData, rxJoyData, ryJoyData);
        return pointing_device_task_user(mouse_report);
    }

    void housekeeping_task_kb(void) {


        for (int row = 0; row < MATRIX_ROWS / 2; row++) {  // 行のループ
            for (int j = 0; j < MATRIX_COLS * 2 - 4; j++) {  // 列のループ
                prev_virtual_keys[0][row][j] = virtual_keys[0][row][j];
                prev_virtual_keys[1][row][j] = virtual_keys[1][row][j];
                prev_virtual_keys[2][row][j] = virtual_keys[2][row][j];
                prev_virtual_keys[3][row][j] = virtual_keys[3][row][j];
            }
        }

         // トラックボールの開放処理

        if (!left_motion_detected && !right_motion_detected) {
            if (time_difference2(tb_current_time, init_time) > INACTIVITY_TIMEOUT) {
                init_time = timer_read();  // init_time を更新

                avg_delta_x = 0;
                avg_delta_y = 0;
                accumulated_h = 0;
                accumulated_v = 0;

                // 移動平均のバッファをリセット
                for (int i = 0; i < MOVING_AVERAGE_SIZE; ++i) {
                    delta_x_buffer[i] = 0;
                    delta_y_buffer[i] = 0;
                }
                buffer_index = 0;

                // キーの状態をリセット
                for (int i = 0; i < MATRIX_COLS * 2 - 4; i++) {
                    if (tb_l_pressed[i]) {
                        unregister_code(virtual_keys[current_layer][0][i]);
                        tb_l_pressed[i] = false;
                    }
                    if (tb_r_pressed[i]) {
                        unregister_code(virtual_keys[current_layer][2][i]);
                        tb_r_pressed[i] = false;
                    }
                    if (tb_d_pressed[i]) {
                        unregister_code(virtual_keys[current_layer][4][i]);
                        tb_d_pressed[i] = false;
                    }
                    if (js_l_pressed[i]) {
                        if (!joyCursorMoveFlagL) {
                            unregister_code(virtual_keys[current_layer][6][i]);
                            js_l_pressed[i] = false;
                        }
                    }
                    if (js_r_pressed[i]) {
                        if (!joyCursorMoveFlagR) {
                            unregister_code(virtual_keys[current_layer][8][i]);
                            js_r_pressed[i] = false;
                        }
                    }

                }
            }
        }
        housekeeping_task_user();
    }


#endif



