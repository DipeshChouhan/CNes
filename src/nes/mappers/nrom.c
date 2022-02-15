#include <stdio.h>
#include "../nes.h"
#include "../mappers.h"


void nrom_read(struct Cpu *cpu) {
    if (cpu->address_bus < 0x4020) {
        common_read(cpu);
        return;
    }

    if (cpu->address_bus > 0x7FFF) {
        cpu->data_bus = cpu->mem[cpu->address_bus];
        return;
    }
}

void nrom_write(struct Cpu *cpu) {
    if (cpu->address_bus < 0x4020) {
        common_write(cpu);
        return;
    }
    if (cpu->address_bus > 0x7FFF) {
        printf("Writing to prg rom. Nrom don't support mapper registers\n");
        exit(1);
    }

    printf("nrom_write to %d\n", cpu->address_bus);
    exit(1);
}

int nrom_chr_read(int vram_addr) {
    return ppu->ptables[vram_addr];
}

void nrom_chr_write(unsigned char data, int vram_addr) {
    ppu->ptables[vram_addr] = data;
}
