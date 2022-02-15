#include "../mappers.h"
#include "../nes.h"

void cnrom_read(struct Cpu *cpu) {

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
}

void cnrom_write(struct Cpu *cpu) {

    if (cpu->address_bus < 0x4020) {
        common_write(cpu);
        return;
    }

    if (cpu->address_bus > 0x7FFF) {
        mapper->registers[0] = cpu->data_bus & 3;
        return;
    }
}


int cnrom_chr_read(int vram_addr) {
    if (mapper->chr_rom_size < 2) {
        return mapper->chr_banks[vram_addr];
    } 
    return mapper->chr_banks[(0x2000 * mapper->registers[0]) + vram_addr];
}

void cnrom_chr_write(unsigned char data, int vram_addr) {
    if (mapper->chr_rom_size == 0) {
        mapper->chr_banks[vram_addr] = data;
    }
}
