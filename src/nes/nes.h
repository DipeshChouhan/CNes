#ifndef _NES_H
#define _NES_H
#include "../mos_6502/cpu_impl.h"
#include <SDL2/SDL.h>
#include "ppu.h"

extern Ppu *ppu;
extern SDL_Event event;

extern int cpu_oamdma_addr;
extern int oamdma_addr;





void power_on_nes(char *game_file);
/* void loadMapper(char *game_file); */


#endif
