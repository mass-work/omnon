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

#include "quantum.h"


#define LAYOUT( \
    L00, L01, L02, L03 \
) \
{ \
    { L00, L01, L02, L03 } \
}

// ジョイスティックのデータを格納する構造体
typedef struct {
    int16_t joyData;         // ジョイスティックのデータ
    uint16_t lastTime1;      // 最初のスクロールタイミング
    uint16_t lastTime2;      // 繰り返しスクロールタイミング
    bool initialScrollDone;  // 初期スクロールが完了したかどうかのフラグ
} JoystickAxisData;

extern int16_t lxOrigin, lyOrigin, rxOrigin, ryOrigin;  // extern宣言を追加

#ifdef CONSOLE_ENABLE
    extern JoystickAxisData rxData;
    extern int8_t debugPMW3360x;
    extern int8_t debugPMW3360y;
#endif


