#include QMK_KEYBOARD_H
#include <stdint.h>
#include <stdbool.h>

// In my case I was using a QMK miryoku layout, this header is where
// some layer definitions come from. However its not needed if you
// aren't using miryoku qmk.
#include "manna-harbour_miryoku.h"

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// README
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// This file is an template/example of how to setup the QMK firmware to control the STM32 video player
// QMK allows us to define a function called process_record_user, this will get called everytime a key is pressed.
// process_record_user allows us to hook in to QMK and run our own code everytime a key is pressed to command the STM32 video player
// The general flow is as follows: Key is pressed on the keyboard, process_recrod_user gets called, send a command over UART to the STM32 video player
// Sections labeled [USER TODO] should be updated to match your QMK layout
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////



////////////////////////////////////////////////////////////////////
// ADD UART DRIVER
////////////////////////////////////////////////////////////////////
// Add QMK UART driver for sending bytes to the STM32
#include "uart.h"
// [USER TODO] Also add the following to rules.mk:
// UART_DRIVER_REQUIRED = yes
////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////




////////////////////////////////////////////////////////////////////////////////////////////
// UART COMMAND MAPPING
////////////////////////////////////////////////////////////////////////////////////////////
// These should match the numbering the STM32 software expects (see: stm32/Core/Inc/app.h)
#define CMD_SEARCH_OPEN 0x01
#define CMD_SEARCH_EXIT 0x02
#define CMD_SEARCH_UP 0x03
#define CMD_SEARCH_DOWN 0x04
#define CMD_SEARCH_SELECT 0x05
#define CMD_BACKSPACE 0x08

#define UART_BAUD_RATE 115200 // baudrate should match what STM32 has set
////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////



///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// focus_stm
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// focus_stm is a state variable used to determine if we should be talking with the STM32 OR with the USB Host (normal keyboard operation)
// if focus_stm = false (not in search mode) - in this state keyboard shoud operate normally. Keypresses will be sent to the USB Host like normal.
// if focus_stm = true (in search mode) - in this state the keyboard is talking with the stm32 video player. Keypresses will result
//                in commands being sent to the stm32 video player instead of the USB Host we are plugged into. By having
//                process_record_user return false, we can tell QMK to not send any keypresses to the USB Host
bool focus_stm = false;
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////



//////////////////////////////////////////////////////////////////////////////////////////
// MAPPING SECTION
//////////////////////////////////////////////////////////////////////////////////////////

// Map QMK baser layer keycodes to commands for the STM32 video player

// [USER TODO] Update the macro definitions in this section with KC macro as it appears in your base layer

// 1. KC to enter search mode ( sends CMD_SEARCH_OPEN )
// I do not have a specific key to press for entering search mode.
// Instead, while not in search mode (normal keyboard operation), I detect a special key combo for entering search mode (Eg. LALT & RALT held, and the 'S' or 'K' key is pressed)
// This is a special case since it only gets detected when we are not in search mode (focus_stm = false). While the rest of the entries below only apply when in search mode (focus_stm = true)
// See the ENTER SEARCH MODE BEHAVIOUR section below for customizing the key combo

// The following apply when we are in search mode (focus_stm = true):

// 2. KC to backspace a character in the search query ( sends CMD_BACKSPACE )
// Eg. pressing the backspace key will result in backspacing a character in the search query, when in search mode
#define ZZ_SRCH_BKSPC LT(U_NUM, KC_BSPC)

// 3. KC to select the entry the cursor is on ( sends CMD_SEARCH_SELECT )
// Eg. pressing enter key will select the video the cursor is on
#define ZZ_SRCH_SELECT_KC LT(U_SYM, KC_ENT)

// 4. KC to exit when in search mode (sends CMD_SEARCH_EXIT )
// Eg. pressing ESC key will exit search mode
#define ZZ_SRCH_EXIT_KC LT(U_MEDIA, KC_ESC)

// 5. Eg. pressing space and tab key (right next to eachother in my layout) will move the search cursor up an down
#define ZZ_SRCH_UP_KC LT(U_NAV, KC_SPC)     // KC to move selection up ( sends CMD_SEARCH_UP )
#define ZZ_SRCH_DOWN_KC LT(U_MOUSE, KC_TAB) // KC to move selection down ( sends CMD_SEARCH_DOWN )

// Map keypresses for sending character input into the search query

// 6. These mappings are used by zz_keycode_to_filename_ascii to map a keypress to a ascii character
// Eg. If we are in search mode, pressing a "LGUI_T(KC_A)" will send the 'a' character to the STM32 over UART this will put the character into the search query
#define ZZ_A LGUI_T(KC_A) // my layout uses homerow mods, so there is where the mod tap KC wrapper (LGUI_T(kc)) comes from
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
#define ZZ_DOT ALGR_T(KC_DOT)
#define ZZ_SLSH LT(U_BUTTON, KC_SLSH)
// TODO: Add support for numbers
//////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////

// Helper function for convering keycode's (kc) to ascii characters using the map from MAPPING SECTION above
static char zz_keycode_to_filename_ascii(uint16_t kc);

bool process_record_user(uint16_t keycode, keyrecord_t *record) {
    ///////////////////////////////////////////////////////////////////////
    // (RUN ONCE) Init UART Peripherial
    ///////////////////////////////////////////////////////////////////////
    static bool run_once = true;
    if (run_once) {
        uart_init(UART_BAUD_RATE);
        run_once = false;
    }
    ///////////////////////////////////////////////////////////////////////
    ///////////////////////////////////////////////////////////////////////



    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // ENTER SEARCH MODE BEHAVIOUR
    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // First gather L/R ALT modifier pressed states, which are used to detect the key combo
    uint8_t mods         = get_mods();
    bool    lalt_pressed = mods & (MOD_BIT(KC_LEFT_ALT));
    bool    ralt_pressed = mods & (MOD_BIT(KC_RIGHT_ALT));

    // Below defines the keycombo to press to enter search mode:
    // - First make sure we are not already in search mode (focus_stm = false).
    // - If LALT and RALT are held down and then
    // - The D key or K key is pressed
    // then we will enter search mode (focus_stm = true) and tell the STM32 video player to open the search GUI

    // Note: We listen for this combo while the keyboard is in normal operation so make sure the combo wont collide with any other system
    //       shortcuts. In my case I chose LALT + RALT + (Key D or Key S) since i knew that was unused
    // Note: in my case im using home row mods which is why the KC_D and KC_K keycodes are wrapped with the mod tap macros, RCTL_T()/LCTL_T()
    // [USER TODO] If a different keyboard combo is desired, update the conditional below:
    if (!focus_stm && lalt_pressed && ralt_pressed && (keycode == LCTL_T(KC_D) || keycode == RCTL_T(KC_K)) && record->event.pressed) {
        // Send the command to the STM32 to enter search mode and open the search GUI
        uart_write(CMD_SEARCH_OPEN);

        // Set focus_stm state to true to indicate search mode is active
        focus_stm = true;

        // nothing else to do, return false since we dont need to send anything to host
        return false;
    }
    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////




    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // DEFINE KEYPRESS HANDLING IN SEARCH MODE
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // Uses the MAPPING SECTION above, when in search mode (focus_stm = true) this checks keycode of the press and sends the correct command to the STM32
    if (focus_stm && record->event.pressed) /* only operate on presses */
    {
        // Eg. if keycode pressed equals ZZ_SRCH_UP_KC then send the search up command
        if (keycode == ZZ_SRCH_UP_KC) {
            // Send command to move selection cursor up
            uart_write(CMD_SEARCH_UP);
        } else if (keycode == ZZ_SRCH_DOWN_KC) {
            // Send command to move selection cursor down
            uart_write(CMD_SEARCH_DOWN);
        } else if (keycode == ZZ_SRCH_SELECT_KC) {
            // Send command to select entry at the cursor
            uart_write(CMD_SEARCH_SELECT);

            // When the STM32 receives this command it will start playing the selected video file and  exit search mode
            focus_stm = false;               // exit search mode
        } else if (keycode == ZZ_SRCH_BKSPC) /* backspace character in search query */
        {
            // Send command to backspace a character in the search query
            uart_write(CMD_BACKSPACE);
        } else if (keycode == ZZ_SRCH_EXIT_KC) {
            // Send command to exit the search mode
            uart_write(CMD_SEARCH_EXIT);

            focus_stm = false; // exit search mode
        } else {
            // else assume the keycode should be a character added to the search query

            // convert the keycode to an ascii character using the map we defined. any invalid keycode will be filtered out here
            char c = zz_keycode_to_filename_ascii(keycode);

            if (c) /* check if c is valid */
            {
                uart_write(c);
            }
            /* else: do nothing */
        }

        /* when focused on stm, return false to not send keypresses to HID Host */
        return false;
    }
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

    return true;
}

// Using the MAPPING SECTION above, maps a KC to a ascii character
static char zz_keycode_to_filename_ascii(uint16_t kc) {
    // return 0 if kc is not mapped/supported
    char c = 0;
    switch (kc) {
        // TODO: Add support for numbers
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
            c = 0; // return 0 on error
            break;
    }

    return c;
}
