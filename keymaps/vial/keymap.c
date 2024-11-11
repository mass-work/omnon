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
#include "quantum.h"


#define LAYOUT( \
    L00, L01, L02, L03, L04, L05, L06, L07, L08, L09, R00, R10, R20, R30, R40, R50, R60, R70, R80, R90, \
    L10, L11, L12, L13, L14, L15, L16, L17, L18, L19, R01, R11, R21, R31, R41, R51, R61, R71, R81, R91, \
    L20, L21, L22, L23, L24, L25, L26, L27, L28, L29, R02, R12, R22, R32, R42, R52, R62, R72, R82, R92, \
    L30, L31, L32, L33, L34, L35, L36, L37, L38, L39, R03, R13, R23, R33, R43, R53, R63, R73, R83, R93, \
    L40, L41, L42, L43, L44, L45, L46, L47, L48, L49, R04, R14, R24, R34, R44, R54, R64, R74, R84, R94, \
    L50, L51, L52, L53, L54, L55, L56, L57, L58, L59, R05, R15, R25, R35, R45, R55, R65, R75, R85, R95, \
    L60, L61, L62, L63, L64, L65, L66, L67, L68, L69, R06, R16, R26, R36, R46, R56, R66, R76, R86, R96, \
    L70, L71, L72, L73, L74, L75, L76, L77, L78, L79, R07, R17, R27, R37, R47, R57, R67, R77, R87, R97, \
    L80, L81, L82, L83, L84, L85, L86, L87, L88, L89, R08, R18, R28, R38, R48, R58, R68, R78, R88, R98, \
    L90, L91, L92, L93, L94, L95, L96, L97, L98, L99, R09, R19, R29, R39, R49, R59, R69, R79, R89, R99  \
) \
{ \
    { L00, L01, L02, L03, L04, L05, L06, L07, L08, L09 }, \
    { L10, L11, L12, L13, L14, L15, L16, L17, L18, L19 }, \
    { L20, L21, L22, L23, L24, L25, L26, L27, L28, L29 }, \
    { L30, L31, L32, L33, L34, L35, L36, L37, L38, L39 }, \
    { L40, L41, L42, L43, L44, L45, L46, L47, L48, L49 }, \
    { L50, L51, L52, L53, L54, L55, L56, L57, L58, L59 }, \
    { L60, L61, L62, L63, L64, L65, L66, L67, L68, L69 }, \
    { L70, L71, L72, L73, L74, L75, L76, L77, L78, L79 }, \
    { L80, L81, L82, L83, L84, L85, L86, L87, L88, L89 }, \
    { L90, L91, L92, L93, L94, L95, L96, L97, L98, L99 }, \
    { R00, R01, R02, R03, R04, R05, R06, R07, R08, R09 }, \
    { R10, R11, R12, R13, R14, R15, R16, R17, R18, R19 }, \
    { R20, R21, R22, R23, R24, R25, R26, R27, R28, R29 }, \
    { R30, R31, R32, R33, R34, R35, R36, R37, R38, R39 }, \
    { R40, R41, R42, R43, R44, R45, R46, R47, R48, R49 }, \
    { R50, R51, R52, R53, R54, R55, R56, R57, R58, R59 }, \
    { R60, R61, R62, R63, R64, R65, R66, R67, R68, R69 }, \
    { R70, R71, R72, R73, R74, R75, R76, R77, R78, R79 }, \
    { R80, R81, R82, R83, R84, R85, R86, R87, R88, R89 }, \
    { R90, R91, R92, R93, R94, R95, R96, R97, R98, R99 } \
}



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
bool readSw1, readSw2, readSw3, readSw4, layerChangActive = false;
const uint8_t row_pins[] = MATRIX_ROW_PINS;
const uint8_t col_pins[] = MATRIX_COL_PINS;

void layerChange(void) {
    if (!layerChangActive) {
        if (!readSw1) {
            if (!readSw2) {
                // layerChangActive = true;
                uprintf("readSw2; %d\n", readSw2);
                layer_move(0);

            } else if (!readSw3) {
                // layerChangActive = true;
                uprintf("readSw3; %d\n", readSw3);
                layer_move(1);
            } else if (!readSw4) {
                // layerChangActive = true;
                uprintf("readSw4; %d\n", readSw4);
                layer_move(2);
            }
        }
    }
}



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
     *
     */
    [0] = LAYOUT(
        KC_BTN1, KC_BTN2, KC_0   , KC_1   , KC_WH_R, KC_WH_L, KC_NO  , KC_NO  , KC_NO  , KC_NO  ,        KC_ENT , KC_BTN3, KC_NO  , KC_NO  , KC_NO  , KC_NO  , KC_NO  , KC_NO  , KC_NO  , KC_NO  ,
        KC_NO  , KC_NO  , KC_5   , KC_0   , KC_WH_U, KC_WH_D, KC_0   , KC_0   , KC_0   , KC_0   ,        KC_NO  , KC_NO  , KC_0   , KC_0   , KC_0   , KC_0   , KC_0   , KC_0   , KC_0   , KC_0   ,
        KC_NO  , KC_NO  , KC_1   , KC_1   , KC_WH_R, KC_WH_L, KC_NO  , KC_NO  , KC_NO  , KC_NO  ,        KC_NO  , KC_NO  , KC_NO  , KC_NO  , KC_NO  , KC_NO  , KC_NO  , KC_NO  , KC_NO  , KC_NO  ,
        KC_NO  , KC_NO  , KC_5   , KC_0   , KC_WH_U, KC_WH_D, KC_0   , KC_0   , KC_0   , KC_0   ,        KC_NO  , KC_NO  , KC_0   , KC_0   , KC_0   , KC_0   , KC_0   , KC_0   , KC_0   , KC_0   ,
        KC_NO  , KC_NO  , KC_1   , KC_1   , KC_RGHT, KC_LEFT, KC_NO  , KC_NO  , KC_NO  , KC_NO  ,        KC_NO  , KC_NO  , KC_NO  , KC_NO  , KC_NO  , KC_NO  , KC_NO  , KC_NO  , KC_NO  , KC_NO  ,
        KC_NO  , KC_NO  , KC_1   , KC_0   , KC_UP  , KC_DOWN, KC_0   , KC_0   , KC_0   , KC_0   ,        KC_NO  , KC_NO  , KC_0   , KC_0   , KC_0   , KC_0   , KC_0   , KC_0   , KC_0   , KC_0   ,
        KC_NO  , KC_NO  , KC_0   , KC_NO  , KC_RGHT, KC_LEFT, KC_NO  , KC_NO  , KC_NO  , KC_NO  ,        KC_NO  , KC_NO  , KC_NO  , KC_NO  , KC_NO  , KC_NO  , KC_NO  , KC_NO  , KC_NO  , KC_NO  ,
        KC_NO  , KC_NO  , KC_5   , KC_NO  , KC_UP  , KC_DOWN, KC_0   , KC_0   , KC_0   , KC_0   ,        KC_NO  , KC_NO  , KC_0   , KC_0   , KC_0   , KC_0   , KC_0   , KC_0   , KC_0   , KC_0   ,
        KC_NO  , KC_NO  , KC_1   , KC_NO  , KC_RGHT, KC_LEFT, KC_NO  , KC_NO  , KC_NO  , KC_NO  ,        KC_NO  , KC_NO  , KC_NO  , KC_NO  , KC_NO  , KC_NO  , KC_NO  , KC_NO  , KC_NO  , KC_NO  ,
        KC_NO  , KC_NO  , KC_5   , KC_NO  , KC_UP  , KC_DOWN, KC_0   , KC_0   , KC_0   , KC_0   ,        KC_NO  , KC_NO  , KC_0   , KC_0   , KC_0   , KC_0   , KC_0   , KC_0   , KC_0   , KC_0
    ),
    [1] = LAYOUT(
        KC_BTN1, KC_BTN2, KC_0   , KC_1   , KC_WH_R, KC_WH_L, KC_NO  , KC_NO  , KC_NO  , KC_NO  ,        KC_ENT , KC_BTN3, KC_NO  , KC_NO  , KC_NO  , KC_NO  , KC_NO  , KC_NO  , KC_NO  , KC_NO  ,
        KC_NO  , KC_NO  , KC_2   , KC_0   , KC_WH_U, KC_WH_D, KC_0   , KC_0   , KC_0   , KC_0   ,        KC_NO  , KC_NO  , KC_0   , KC_0   , KC_0   , KC_0   , KC_0   , KC_0   , KC_0   , KC_0   ,
        KC_NO  , KC_NO  , KC_1   , KC_1   , KC_WH_R, KC_WH_L, KC_NO  , KC_NO  , KC_NO  , KC_NO  ,        KC_NO  , KC_NO  , KC_NO  , KC_NO  , KC_NO  , KC_NO  , KC_NO  , KC_NO  , KC_NO  , KC_NO  ,
        KC_NO  , KC_NO  , KC_2   , KC_0   , KC_WH_U, KC_WH_D, KC_0   , KC_0   , KC_0   , KC_0   ,        KC_NO  , KC_NO  , KC_0   , KC_0   , KC_0   , KC_0   , KC_0   , KC_0   , KC_0   , KC_0   ,
        KC_NO  , KC_NO  , KC_2   , KC_1   , KC_RGHT, KC_LEFT, KC_NO  , KC_NO  , KC_NO  , KC_NO  ,        KC_NO  , KC_NO  , KC_NO  , KC_NO  , KC_NO  , KC_NO  , KC_NO  , KC_NO  , KC_NO  , KC_NO  ,
        KC_NO  , KC_NO  , KC_1   , KC_0   , KC_UP  , KC_DOWN, KC_0   , KC_0   , KC_0   , KC_0   ,        KC_NO  , KC_NO  , KC_0   , KC_0   , KC_0   , KC_0   , KC_0   , KC_0   , KC_0   , KC_0   ,
        KC_NO  , KC_NO  , KC_1   , KC_NO  , KC_RGHT, KC_LEFT, KC_LSFT, KC_NO  , KC_NO  , KC_LSFT,        KC_NO  , KC_NO  , KC_NO  , KC_NO  , KC_LSFT, KC_NO  , KC_NO  , KC_LSFT, KC_NO  , KC_NO  ,
        KC_NO  , KC_NO  , KC_5   , KC_NO  , KC_UP  , KC_DOWN, KC_0   , KC_0   , KC_0   , KC_0   ,        KC_NO  , KC_NO  , KC_0   , KC_0   , KC_0   , KC_0   , KC_0   , KC_0   , KC_0   , KC_0   ,
        KC_NO  , KC_NO  , KC_0   , KC_NO  , KC_RGHT, KC_LEFT, KC_BTN3, KC_NO  , KC_NO  , KC_BTN3,        KC_NO  , KC_NO  , KC_NO  , KC_NO  , KC_BTN3, KC_NO  , KC_NO  , KC_BTN3, KC_NO  , KC_NO  ,
        KC_NO  , KC_NO  , KC_5   , KC_NO  , KC_UP  , KC_DOWN, KC_0   , KC_0   , KC_0   , KC_0   ,        KC_NO  , KC_NO  , KC_0   , KC_0   , KC_0   , KC_0   , KC_0   , KC_0   , KC_0   , KC_0
    ),
    [2] = LAYOUT(
        _______, _______, _______, _______, _______, _______, _______, _______, _______, _______,        _______, _______, _______, _______, _______, _______, _______, _______, _______, _______,
        _______, _______, _______, _______, _______, _______, _______, _______, _______, _______,        _______, _______, _______, _______, _______, _______, _______, _______, _______, _______,
        _______, _______, _______, _______, _______, _______, _______, _______, _______, _______,        _______, _______, _______, _______, _______, _______, _______, _______, _______, _______,
        _______, _______, _______, _______, _______, _______, _______, _______, _______, _______,        _______, _______, _______, _______, _______, _______, _______, _______, _______, _______,
        _______, _______, _______, _______, _______, _______, _______, _______, _______, _______,        _______, _______, _______, _______, _______, _______, _______, _______, _______, _______,
        _______, _______, _______, _______, _______, _______, _______, _______, _______, _______,        _______, _______, _______, _______, _______, _______, _______, _______, _______, _______,
        _______, _______, _______, _______, _______, _______, _______, _______, _______, _______,        _______, _______, _______, _______, _______, _______, _______, _______, _______, _______,
        _______, _______, _______, _______, _______, _______, _______, _______, _______, _______,        _______, _______, _______, _______, _______, _______, _______, _______, _______, _______,
        _______, _______, _______, _______, _______, _______, _______, _______, _______, _______,        _______, _______, _______, _______, _______, _______, _______, _______, _______, _______,
        _______, _______, _______, _______, _______, _______, _______, _______, _______, _______,        _______, _______, _______, _______, _______, _______, _______, _______, _______, _______
    )
};

// uint8_t current_layer;
uint16_t virtual_keys[4][MATRIX_ROWS / 2][MATRIX_COLS * 2 - 4];
uint16_t import_keymaps[4][MATRIX_ROWS / 2][MATRIX_COLS * 2];

void load_virtual_keys(void) {
    for (int row = 0; row < MATRIX_ROWS / 2; row++) {  // 行のループ
        for (int i = 0; i < 2; i++) {  // 列のループ
            for (int j = 0; j < MATRIX_COLS; j++) {  // 列のループ
                if (i == 0) {
                    import_keymaps[0][row][j] = keymap_key_to_keycode(0, (keypos_t){.row = row, .col = j});
                    import_keymaps[1][row][j] = keymap_key_to_keycode(1, (keypos_t){.row = row, .col = j});
                    import_keymaps[2][row][j] = keymap_key_to_keycode(2, (keypos_t){.row = row, .col = j});
                    import_keymaps[3][row][j] = keymap_key_to_keycode(3, (keypos_t){.row = row, .col = j});
                } else {
                    import_keymaps[0][row][j + (MATRIX_ROWS / 2)] = keymap_key_to_keycode(0, (keypos_t){.row = j + (MATRIX_ROWS / 2), .col = row});
                    import_keymaps[1][row][j + (MATRIX_ROWS / 2)] = keymap_key_to_keycode(1, (keypos_t){.row = j + (MATRIX_ROWS / 2), .col = row});
                    import_keymaps[2][row][j + (MATRIX_ROWS / 2)] = keymap_key_to_keycode(2, (keypos_t){.row = j + (MATRIX_ROWS / 2), .col = row});
                    import_keymaps[3][row][j + (MATRIX_ROWS / 2)] = keymap_key_to_keycode(3, (keypos_t){.row = j + (MATRIX_ROWS / 2), .col = row});
                }
            }
        }
    }
    for (int row = 0; row < MATRIX_ROWS / 2; row++) {  // 行のループ
        int i = 0;
        for (int col = 0; col < MATRIX_COLS * 2; col++) {  // 列のループ
            if (col == 0 || col == 1 || col == 10 || col == 11) {
                continue; // 該当する列は飛ばす
            }
            virtual_keys[0][row][i] = import_keymaps[0][row][col];
            virtual_keys[1][row][i] = import_keymaps[1][row][col];
            virtual_keys[2][row][i] = import_keymaps[2][row][col];
            virtual_keys[3][row][i] = import_keymaps[3][row][col];
            i++;
        }
    }
}

void keyboard_post_init_user(void) {
    #ifdef CONSOLE_ENABLE
        debug_enable=true;
        debug_matrix=true;
        debug_mouse=true;
    #endif
}

void matrix_scan_user(void) {
    load_virtual_keys();
    readSw1 = readPin(col_pins[0]);
    readSw2 = readPin(col_pins[1]);
    readSw3 = readPin(row_pins[0]);
    readSw4 = readPin(row_pins[1]);
    // uprintf("readSw1; %d\n", readSw1);
    layerChange();


    #ifdef CONSOLE_ENABLE
        #include <print.h>
        #include "wait.h"
            // uprintf("Layer 1, Key %d: %d\n", 999 , get_highest_layer(layer_state));
            // uprintf("Layer 1, Key %d: %d\n", 0 , virtual_keys[0][0][0 ]);
            // uprintf("Layer 1, Key %d: %d\n", 1 , virtual_keys[0][0][1 ]);
            // uprintf("Layer 1, Key %d: %d\n", 2 , virtual_keys[0][0][2 ]);
            // uprintf("Layer 1, Key %d: %d\n", 3 , virtual_keys[0][0][3 ]);
            // uprintf("Layer 1, Key %d: %d\n", 4 , virtual_keys[0][1][2 ]);
            // uprintf("Layer 1, Key %d: %d\n", 10, virtual_keys[0][1][3 ]);
            // uprintf("Layer 1, Key %d: %d\n", 11, virtual_keys[0][0][11]);
            // uprintf("Layer 1, Key %d: %d\n", 18, virtual_keys[0][0][18]);
            // uprintf("Layer 1, Key %d: %d\n", 19, virtual_keys[0][0][19]);
            // wait_ms(1000);
            // uprintf("Layer 2, Key %d: %d\n", 5, layer_2_virtual_keys[0]);
            // 現在のレイヤーが0で、行0列4のキーがKC_LCTLかどうかを確認
            // if (layer_state_is(0)) {  // レイヤー0がアクティブか確認
            //     uint16_t keycode = keymap_key_to_keycode(0, (keypos_t){.row = 0, .col = 4});
            //     if (keycode == KC_LCTL) {
            //         uprintf(" %d\n", keycode);
            //         wait_us(10);
            //     }
            // }
            // for (int i = 0; i < NUM_VIRTUAL_KEYS; i++) {
            //     uprintf("Layer 0, Key %d: %d\n", i + 5, layer_0_virtual_keys[i]);
            //     uprintf("Layer 1, Key %d: %d\n", i + 5, layer_1_virtual_keys[i]);
            //     uprintf("Layer 2, Key %d: %d\n", i + 5, layer_2_virtual_keys[i]);
            // }

    #endif
}


