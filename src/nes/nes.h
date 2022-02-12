#ifndef _NES_H
#define _NES_H
#include "../mos_6502/cpu_impl.h"
#include <SDL2/SDL.h>
#include "ppu.h"

extern Ppu *ppu;
extern SDL_Event event;

extern int cpu_oamdma_addr;
extern int oamdma_addr;

extern unsigned char joypad1;
extern unsigned char joypad2;
/* extern */ 


#define BUTTON_A        0b00000001;
#define BUTTON_B        0b00000010;
#define BUTTON_SELECT   0b00000100;
#define BUTTON_START    0b00001000;
#define BUTTON_UP       0b00010000;
#define BUTTON_DOWN     0b00100000;
#define BUTTON_LEFT     0b01000000;
#define BUTTON_RIGHT    0b10000000;

#define JOYPAD_SET(_joypad, _button)    \
    _joypad |= _button;                 \

#define JOYPAD_CLEAR(_joypad, _button)      \
    _joypad &= ~_button;                    \


void power_on_nes(char *game_file);
/* void loadMapper(char *game_file); */


#endif
