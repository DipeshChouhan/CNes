// TODO: Implement OAMDMA
#include "./nes1/nes.h"
#include "./nes1/ppu.h"
#include "mos_6502/cpu_impl.h"
#include <SDL2/SDL.h>

#define DEBUG_ON
int main(int argc, char* argv[]) {
    power_on_nes("../test/Contra (U).nes");

    return 0;
    
}

