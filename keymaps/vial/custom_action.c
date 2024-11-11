/*
Copyright 2012,2013 Jun Wako <wakojun@gmail.com>

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

#include "quantum.h"
#include "custom_action.h"

// custom
void custom_register_mouse(uint8_t mouse_keycode, bool pressed) {
#ifdef CONSOLE_ENABLE
    printf("custom_register_mouse called with keycode: %d, pressed: %d\n", mouse_keycode, pressed);

#endif
#ifdef MOUSEKEY_ENABLE
    if (pressed) {
        mousekey_on(mouse_keycode);
    } else {
        mousekey_off(mouse_keycode);
    }

    switch (mouse_keycode) {
#    if defined(PS2_MOUSE_ENABLE) || defined(POINTING_DEVICE_ENABLE)
        case KC_MS_BTN1 ... KC_MS_BTN8:
#        if defined(POINTING_DEVICE_ENABLE)
// #           if !defined(MOUSEKEY_ENABLE)
                pointing_device_keycode_handler(mouse_keycode, pressed);
// #           endif
#        endif
            break;
#    endif
        default:
            mousekey_send();
            break;
    }
#elif defined(POINTING_DEVICE_ENABLE)
    if (IS_MOUSE_KEYCODE(mouse_keycode)) {
// #           if !defined(MOUSEKEY_ENABLE)
                pointing_device_keycode_handler(mouse_keycode, pressed);
// #           endif
    }
#endif

#ifdef PS2_MOUSE_ENABLE
    if (KC_MS_BTN1 <= mouse_keycode && mouse_keycode <= KC_MS_BTN3) {
        uint8_t tmp_button_msk = MOUSE_BTN_MASK(mouse_keycode - KC_MS_BTN1);
        tp_buttons             = pressed ? tp_buttons | tmp_button_msk : tp_buttons & ~tmp_button_msk;
    }
#endif
}

