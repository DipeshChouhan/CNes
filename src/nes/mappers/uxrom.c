#include "../mappers.h"
#include "../nes.h"

void uxrom_read(struct Cpu *cpu) {
    if (cpu->address_bus < 0x4020) {
        common_read(cpu);
        return;
    }

    if (cpu->address_bus > 0xBFFF) {
        cpu->data_bus = mapper->prg_banks[(0x4000 * mapper->prg_rom_size) + 
        (cpu->address_bus & 0x3FFF)];
    } else if (cpu->address_bus > 0x7FFF) {
        // switchable bank
        cpu->data_bus = mapper->prg_banks[(0x4000 * mapper->registers[0]) + (cpu->address_bus & 0x3FFF)];
    }

}

void uxrom_write(struct Cpu *cpu) {
    if (cpu->address_bus < 0x4020) {
        common_write(cpu);
        return;
    }

    if (cpu->address_bus > 0x7FFF) {
        mapper->registers[0] = cpu->data_bus & mapper->registers[1];
        return;
    }
}

int uxrom_chr_read(int vram_addr) {
    return mapper->chr_banks[vram_addr];
}

void uxrom_chr_write(unsigned char data, int vram_addr) {
    if (mapper->chr_rom_size == 0) {
        mapper->chr_banks[vram_addr] = data;
    }
}
