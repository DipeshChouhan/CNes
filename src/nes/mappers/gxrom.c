#include "../mappers.h"
#include <stdio.h>

#define CHR_BANK 0
#define PRG_BANK 1

void gxrom_read(struct Cpu *cpu) {
    if (cpu->address_bus < 0x4020) {
        common_read(cpu);
        return;
    }

    if (cpu->address_bus > 0x7FFF) {
        cpu->data_bus = mapper->prg_banks[(0x8000 * mapper->registers[PRG_BANK]) + (cpu->address_bus & 0x7FFF)];
    } 
}

void gxrom_write(struct Cpu *cpu) {
    if (cpu->address_bus < 0x4020) {
        common_write(cpu);
        return;
    }

    if (cpu->address_bus > 0x7FFF) {
        mapper->registers[CHR_BANK] = cpu->data_bus & 3;
        mapper->registers[PRG_BANK] = (cpu->data_bus >> 4) & 3;
    }
}

int gxrom_chr_read(int vram_addr) {
    if (mapper->chr_rom_size < 2) {
        return mapper->chr_banks[vram_addr];
    }
    return mapper->chr_banks[(0x2000 * mapper->registers[CHR_BANK]) + vram_addr];
}

void gxrom_chr_write(unsigned char data, int vram_addr) {
    if (mapper->chr_rom_size == 0) {
        // chr ram
        mapper->chr_banks[vram_addr] = data;
        return;
    }
}
