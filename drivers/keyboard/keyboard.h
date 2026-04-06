#ifndef KEYBOARD_H
#define KEYBOARD_H

#include <kernel/types.h>

/* PS/2 keyboard I/O ports */
#define KBD_DATA_PORT     0x60
#define KBD_STATUS_PORT   0x64
#define KBD_CMD_PORT      0x64

/* Special key codes */
#define KEY_ESCAPE      0x01
#define KEY_BACKSPACE   0x0E
#define KEY_TAB         0x0F
#define KEY_ENTER       0x1C
#define KEY_CTRL        0x1D
#define KEY_LSHIFT      0x2A
#define KEY_RSHIFT      0x36
#define KEY_ALT         0x38
#define KEY_CAPS        0x3A
#define KEY_F1          0x3B
#define KEY_F12         0x58
#define KEY_DELETE      0x53

/* Key event */
typedef struct {
    uint8_t scancode;
    char    ascii;
    bool    pressed;      /* true = key down, false = key up */
    bool    shift;
    bool    ctrl;
    bool    alt;
} key_event_t;

#define KBD_BUFFER_SIZE  256

void       keyboard_init(void);
bool       keyboard_poll(key_event_t *event);
char       keyboard_getchar(void);   /* Blocking */

/* Keyboard driver registration */
void keyboard_driver_register(void);

#endif /* KEYBOARD_H */
