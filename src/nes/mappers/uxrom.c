#include "../mappers.h"
#include "../nes.h"

void uxrom_read(struct Cpu *cpu) {
    if (cpu->address_bus < 0x4020) {
        common_read(cpu);
        return;
    }

    if (cpu->address_bus > 0xBFFF) {
        cpu->data_bus = cpu->mem[cpu->address_bus];
    } else if (cpu->address_bus > 0x7FFF) {
        // switchable bank
        if (mapper->registers[0]  == mapper->prg_rom_size) {
            cpu->data_bus = cpu->mem[cpu->address_bus + 0x4000];
            return;
        }
        cpu->data_bus = mapper->prg_banks[(0x4000 * mapper->registers[0]) + (cpu->address_bus % 0x4000)];
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
    printf("uxrom_write to 0x%X\n", cpu->address_bus);
    /* exit(1); */
}

int uxrom_chr_read(int vram_addr) {
    return ppu->ptables[vram_addr];
}

void uxrom_chr_write(unsigned char data, int vram_addr) {
    ppu->ptables[vram_addr] = data;
}
