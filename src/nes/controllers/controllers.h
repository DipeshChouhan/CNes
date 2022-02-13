#ifndef _CONTROLLERS_H
#define _CONTROLLERS_H

extern unsigned char joypad1;
extern unsigned char joypad2;
extern unsigned char joypad1_read_count;


#define JOYPAD_SET(_joypad, _button)    \
    _joypad |= _button;                 \

#define JOYPAD_CLEAR(_joypad, _button)      \
    _joypad &= ~_button;                    \


void joypad1_events();

#endif
