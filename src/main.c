// TODO: Implement OAMDMA
#include "./nes/nes.h"
#include "./nes/ppu.h"
#include "mos_6502/cpu_impl.h"
#include <SDL2/SDL.h>

#define DEBUG_ON
int main(int argc, char* argv[]) {
    power_on_nes("../test/Metal Gear (USA).nes");
    return 0;
    
}

