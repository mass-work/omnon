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

#include "omnon.h"
#include "config.h"

#if JOYSTICK_MODE == 0
    #include "../../drivers/analog_joystick.h"
#elif JOYSTICK_MODE == 1
    // joystick_config_t の初期化
    // 初期化がハードコーディングのため修正が必要
    joystick_config_t joystick_axes[JOYSTICK_AXIS_COUNT] = {
        JOYSTICK_AXIS_IN(GP28, 285, 575, 900),
        JOYSTICK_AXIS_IN(GP29, 285, 575, 900),
        JOYSTICK_AXIS_IN(GP26, 285, 575, 900),
        JOYSTICK_AXIS_IN(GP27, 285, 575, 900)
    };
#endif

const uint16_t PROGMEM keymaps[][MATRIX_ROWS][MATRIX_COLS] = {
    /* Thanks for choosing Omnon! We're thrilled to have you here.
     *   ____________________________
     *  /   _______   　   _______   \
     * |   |       |  　  |       |   |
     * |   |       |  　  |       |   |
     * |   |_______|  　  |_______|   |
     * |              ｗ              |
     * |    _______   　   _______    |
     * |   |       |  ➀  |       |   |
     * |   |   3   |  ➁  |   4   |   |
     * |   |_______|  　  |_______|   |
     *  \____________________________/
     */
    [0] = LAYOUT(
        KC_BTN1, KC_BTN2, KC_ENT, KC_BTN3
    )
};

// #ifdef CONSOLE_ENABLE
//     #include <print.h>
//     #include "wait.h"

//     void keyboard_post_init_user(void) {
//     debug_enable=true;
//     debug_matrix=true;
//     debug_mouse=true;
//     }

//     void matrix_scan_user(void) {
//         uprintf(" %d\n %d\n", debugPMW3360x, debugPMW3360y);
//         wait_us(10);
//     }
// #endif
