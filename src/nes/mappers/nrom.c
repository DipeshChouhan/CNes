#include <stdio.h>
#include "../nes.h"
#include "../mappers.h"


void nrom_read(struct Cpu *cpu) {
    if (cpu->address_bus < 0x4020) {
        common_read(cpu);
        return;
    }

    /* if (cpu->address_bus < 0xC000) { */
    /*     printf("error\n"); */
    /*     exit(1); */
    /* } */

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

void nrom_vram_read(struct Cpu *cpu, int vram_addr) {

    if (vram_addr < 0x2000) {
        cpu->data_bus = ppu->read_buffer;
        ppu->read_buffer = ppu->ptables[ppu->v & 0x3FFF];
        return;
    }
    cpu->data_bus = ppu->read_buffer;
    // TODO: Four screen mirroring
    ppu->read_buffer = ppu->nametables[ppu->get_mirrored_addr(ppu->v & 0xFFF)];
}

void nrom_vram_write(struct Cpu *cpu, int vram_addr) {

    if (vram_addr < 0x2000) {
        ppu->ptables[ppu->v & 0x3FFF] = cpu->data_bus;
        return;
    }

    ppu->nametables[ppu->get_mirrored_addr(ppu->v & 0xFFF)] = cpu->data_bus;

}
