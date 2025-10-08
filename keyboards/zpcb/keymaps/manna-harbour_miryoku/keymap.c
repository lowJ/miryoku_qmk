// Copyright 2019 Manna Harbour
// https://github.com/manna-harbour/miryoku

// This program is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 2 of the License, or (at your option) any later version. This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with this program. If not, see <http://www.gnu.org/licenses/>.

#include QMK_KEYBOARD_H
#include <stdint.h>
#include <stdbool.h>

#include "manna-harbour_miryoku.h"
#include "uart.h"
enum custom_keycodes {
    KC_SRCH = SAFE_RANGE,
};
//Special commands
//TODO: check these enumerations
#define CMD_SEARCH_OPEN 0x01
#define CMD_SEARCH_EXIT 0x02
#define CMD_SEARCH_UP 0x03
#define CMD_SEARCH_DOWN 0x04
#define CMD_SEARCH_SELECT 0x05

// Map Base layer keycodes here
//put char into search query
#define ZZ_A LGUI_T(KC_A)
#define ZZ_B KC_B
#define ZZ_C KC_C
#define ZZ_D LCTL_T(KC_D)
#define ZZ_E KC_E
#define ZZ_F LSFT_T(KC_F)
#define ZZ_G KC_G
#define ZZ_H KC_H
#define ZZ_I KC_I
#define ZZ_J RSFT_T(KC_J)
#define ZZ_K RCTL_T(KC_K)
#define ZZ_L RALT_T(KC_L)
#define ZZ_M KC_M 
#define ZZ_N KC_N
#define ZZ_O KC_O
#define ZZ_P KC_P
#define ZZ_Q KC_Q
#define ZZ_R KC_R
#define ZZ_S LALT_T(KC_S)
#define ZZ_T KC_T
#define ZZ_U KC_U
#define ZZ_V KC_V
#define ZZ_W KC_W
#define ZZ_X ALGR_T(KC_X)
#define ZZ_Y KC_Y
#define ZZ_Z LT(U_BUTTON, KC_Z)
#define ZZ_BKSPC LT(U_NUM, KC_BSPC)
#define ZZ_DOT ALGR_T(KC_DOT)
#define ZZ_SLSH LT(U_BUTTON, KC_SLSH)

// navigation related
// KC to select video
#define ZZ_ENTER LT(U_SYM, KC_ENT) 

// KC to exit when in search mode 
#define ZZ_ESC LT(U_MEDIA, KC_ESC)

// KC to move selection up
#define ZZ_SRCH_UP_KC LT(U_NAV, KC_SPC)

// KC to move selection down
#define ZZ_SRCH_DOWN_KC  LT(U_MOUSE,KC_TAB)

char zz_keycode_to_filename_ascii( uint16_t kc );

bool process_record_user(uint16_t keycode, keyrecord_t *record) {
    static bool run_once = true;
    static bool focus_stm = false;

    /* TODO: better hook for this? */
    if( run_once )
    {
        uart_init( 115200 ); //faster?
        run_once = false;
    }


//#define MOD_MASK_CTRL (MOD_BIT(KC_LEFT_CTRL) | MOD_BIT(KC_RIGHT_CTRL))
//#define MOD_MASK_SHIFT (MOD_BIT(KC_LEFT_SHIFT) | MOD_BIT(KC_RIGHT_SHIFT))
//#define MOD_MASK_ALT (MOD_BIT(KC_LEFT_ALT) | MOD_BIT(KC_RIGHT_ALT))
//#define MOD_MASK_GUI (MOD_BIT(KC_LEFT_GUI) | MOD_BIT(KC_RIGHT_GUI))

    uint8_t mods = get_mods();
    bool lalt_pressed = mods & (MOD_BIT(KC_LEFT_ALT)) ;
    bool ralt_pressed = mods & (MOD_BIT(KC_RIGHT_ALT)) ;

    // CMD_SEARCH_OPEN / CMD_SEARCH_EXIT
    // Details: TODO
    // Eg: I have this confiured if LALT and RALT pressed at the same time THEN a ctrl 
    if( lalt_pressed && ralt_pressed && record->event.pressed && (keycode == LCTL_T(KC_D) || keycode == RCTL_T(KC_K)) )
    {
        // If we are in search mode already, we should exit
        if( focus_stm )
        {
            uart_write(CMD_SEARCH_EXIT);
            focus_stm = false;
        }
        else // We are not in search mode, we should open search mode
        {
            uart_write(CMD_SEARCH_OPEN);
            focus_stm = true;
        }

        // nothing else to do, return false since we dont need to send anything to host
        return false;
    }

    // Only need to act on keypresses if we are in search mode
    if( focus_stm && record->event.pressed ) /* only operate on presses */
    {
        if( keycode == ZZ_SRCH_UP_KC )
        {
            uart_write(CMD_SEARCH_UP);
        }
        else if(keycode == ZZ_SRCH_DOWN_KC )
        {
            uart_write(CMD_SEARCH_DOWN );
        }
        else if( keycode == ZZ_ENTER ) /* */
        {
            uart_write( CMD_SEARCH_SELECT ); /* send select cmd and exit search */
            focus_stm = false;
        }
        else if( keycode == ZZ_BKSPC ) /* backspace character in search query */
        {
            uart_write( 0x08 ); /* ascii backspace */
        }
        else if( keycode == ZZ_ESC ) /* escape can exit search */
        {
            uart_write(CMD_SEARCH_EXIT);
            focus_stm = false;
        }
        else /* put characters into search query */
        {
            /* TODO: maybe we shouldn't filter any chars here? Filtering can be done on stm */
            char c = zz_keycode_to_filename_ascii(keycode);

            if( c ) /* check if c is valid */
            {
                uart_write( c );
            }
            /* else: do nothing */

        }

        /* when focused on stm, return false to not send keypresses to HID Host */
        return false;
    }


    return true;

}

/* TODO: support case sensitivity later */
char zz_keycode_to_filename_ascii( uint16_t kc )
{
    char c = 0;
    switch (kc)
    {
        case ZZ_A:
            c = 'a';
            break;
        case ZZ_B:
            c = 'b';
            break;
        case ZZ_C:
                c = 'c';
                break;
        case ZZ_D:
                c = 'd';
                break;
        case ZZ_E:
                c = 'e';
                break;
        case ZZ_F:
                c = 'f';
                break;
        case ZZ_G:
                c = 'g';
                break;
        case ZZ_H:
                c = 'h';
                break;
        case ZZ_I:
                c = 'i';
                break;
        case ZZ_J:
                c = 'j';
                break;
        case ZZ_K:
                c = 'k';
                break;
        case ZZ_L:
                c = 'l';
                break;
        case ZZ_M:
                c = 'm';
                break;
        case ZZ_N:
                c = 'n';
                break;
        case ZZ_O:
                c = 'o';
                break;
        case ZZ_P:
                c = 'p';
                break;
        case ZZ_Q:
                c = 'q';
                break;
        case ZZ_R:
                c = 'r';
                break;
        case ZZ_S:
                c = 's';
                break;
        case ZZ_T:
                c = 't';
                break;
        case ZZ_U:
                c = 'u';
                break;
        case ZZ_V:
                c = 'v';
                break;
        case ZZ_W:
                c = 'w';
                break;
        case ZZ_X:
                c = 'x';
                break;
        case ZZ_Y:
                c = 'y';
                break;
        case ZZ_Z:
                c = 'z';
                break;
        case ZZ_DOT:
            c = '.';
            break;
        case ZZ_SLSH:
            c = '/';
            break;
        default:
            // unknown
            c = 0;
            break;
    }


    return c;

}
