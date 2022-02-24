// TODO: Implement OAMDMA
#include "./nes/nes.h"
#include "./nes/ppu.h"
#include "mos_6502/cpu_impl.h"
#include <SDL2/SDL.h>


int main(int argc, char* argv[]) {
    power_on_nes("../test/Adventures of Lolo 2 (USA).nes");
    return 0;
}
