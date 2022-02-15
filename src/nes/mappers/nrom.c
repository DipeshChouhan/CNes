#include <stdio.h>
#include "../nes.h"
#include "../mappers.h"


void nrom_read(struct Cpu *cpu) {
    if (cpu->address_bus < 0x4020) {
        common_read(cpu);
        return;
    }

    if (mapper->prg_rom_size == 1) {
        if (cpu->address_bus > 0x7FFF) {
            cpu->data_bus = mapper->prg_banks[cpu->address_bus & 0x3FFF];
        }
        return;
    }

    if (cpu->address_bus > 0x7FFF) {
        cpu->data_bus = mapper->prg_banks[cpu->address_bus & 0x7FFF];
    }
    return;
}

void nrom_write(struct Cpu *cpu) {
    if (cpu->address_bus < 0x4020) {
        common_write(cpu);
        return;
    }
}

int nrom_chr_read(int vram_addr) {
    return mapper->chr_banks[vram_addr];
}

void nrom_chr_write(unsigned char data, int vram_addr) {
    if (mapper->chr_rom_size == 0) {
        mapper->chr_banks[vram_addr] = data;
        return;
    }
}
