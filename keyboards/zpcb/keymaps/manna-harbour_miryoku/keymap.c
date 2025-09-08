// Copyright 2019 Manna Harbour
// https://github.com/manna-harbour/miryoku

// This program is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 2 of the License, or (at your option) any later version. This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with this program. If not, see <http://www.gnu.org/licenses/>.

#include QMK_KEYBOARD_H
#include <stdint.h>
#include <stdbool.h>

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

char keycode_to_filename_ascii( uint16_t kc , bool is_shift );

bool process_record_user(uint16_t keycode, keyrecord_t *record) {
    static bool run_once = true;
    static bool focus_stm = false;

    /* TODO: better function for init? */
    if( run_once )
    {
        uart_init( 115200 );
        run_once = false;
    }

    if( keycode == KC_SRCH && record->event.pressed )
    {
        if( focus_stm )
        {
            uart_write(CMD_SEARCH_EXIT);
            focus_stm = false;
        }
        else
        {
            uart_write(CMD_SEARCH_OPEN);
            focus_stm = true;
        }
    }

    if( focus_stm && record->event.pressed ) /* only operate on presses */
    {
        uint8_t mods = get_mods();
        bool shift_pressed = (mods & MOD_MASK_SHIFT); /* TODO: test this */
        bool ctrl_pressed = ( mods & MOD_MASK_CTRL); /* TODO: test this */

        if( ctrl_pressed ) /* ctrl layer handles up down select commands */
        {
            switch ( keycode ) {
                /* TODO: make these #defines, easier to configure */
                case KC_P:
                    uart_write( CMD_SEARCH_UP);
                    break;
                case KC_N:
                    uart_write( CMD_SEARCH_DOWN );
                    break;
                case KC_ENT:
                    uart_write( CMD_SEARCH_SELECT );
                    break;
            }
        }
        else if( keycode == KC_BSPC ) /* backspace character in query */
        {
            uart_write( 0x08 ); /* ascii backspace */
        }
        else if( keycode == KC_ESC ) /* escape can exit search */
        {
            uart_write(CMD_SEARCH_EXIT);
            focus_stm = false;
        }
        else /* put characters into search query */
        {
            /* TODO: maybe we shouldn't filter any chars here? Filtering can be done on stm */
            char c = keycode_to_filename_ascii(keycode, shift_pressed);

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

char keycode_to_filename_ascii( uint16_t kc , bool is_shift )
{

//Letters:
//KC_A = 0x04
//KC_Z = 0x1d
//Numbers:
//KC_1 = 0x1e
//KC_0 = 0x27
//Others:
//KC_DOT
//KC_UNDERSCORE
//KC_SLSH /* added to support file paths */

    // Handle letter keys
    if( kc >= KC_A && kc <= KC_Z )
    {
        if( is_shift )
        {
            return ('a' + (kc - KC_A));
        }
        else
        {
            return ('A' + (kc - KC_A));
        }

    }

    // Handle number keys
    if( kc >= KC_1 && kc <= KC_0 )
    {
        if( ! is_shift )
        {
            if( KC_0 )
            {
                return '0';
            }
            else
            {
                return '1' + (kc = KC_1);

            }
        }
    }

    // Others
    if( kc == KC_DOT )
    {
        if( ! is_shift )
        {
            return '.';
        }
    }

    if (kc == KC_UNDERSCORE )
    {
        if( is_shift )
        {
            return '_';
        }
        else
        {
            return '-';
        }
    }

    if( kc == KC_SLSH )
    {
        if( ! is_shift )
        {
            return '/';
        }
    }

    // Unknown
    return 0x00;

}
