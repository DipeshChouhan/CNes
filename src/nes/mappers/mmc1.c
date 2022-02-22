#include "../nes.h"
#include "../mappers.h"

#define SHIFT_REG 0
#define PRG_MODE 2
#define CHR_MODE 3
#define CHR_0 4
#define CHR_1 5
#define PRG 6
#define W_COUNT 7   // write count


void mmc1_read(struct Cpu *cpu) {
    if (cpu->address_bus < 0x4020) {
        common_read(cpu);
        return;
    }

    if (cpu->address_bus < 0x6000) return;


    if (cpu->address_bus < 0x8000 && cpu->address_bus > 0x5FFF) {
        if (mapper->have_prg_ram) {
            cpu->data_bus = mapper->prg_ram[cpu->address_bus & 0x1FFF];
        }
        return;
    }

    if (mapper->registers[PRG_MODE] < 2) {
        // 0, 1: switch 32 KB at $8000, ignoring low bit of bank number;
        cpu->data_bus = mapper->prg_banks[
            (0x8000 * (mapper->registers[PRG] >> 1)) + (cpu->address_bus & 0x7FFF)];
    } else if (mapper->registers[PRG_MODE] == 2) {
        // 2: fix first bank at $8000 and switch 16 KB bank at $C000;
        if (cpu->address_bus > 0xBFFF) {
            cpu->data_bus = mapper->prg_banks[(0x4000 * mapper->registers[PRG]) + 
            (cpu->address_bus & 0x3FFF)];
        } else {
            cpu->data_bus = mapper->prg_banks[cpu->address_bus & 0x3FFF];
        }
    } else {
        // 3: fix last bank at $C000 and switch 16 KB bank at $8000)
        if (cpu->address_bus > 0xBFFF) {
            cpu->data_bus = mapper->prg_banks[(0x4000 * mapper->prg_rom_size) + 
            (cpu->address_bus & 0x3FFF)];
        } else {
            cpu->data_bus = mapper->prg_banks[(0x4000 * mapper->registers[PRG]) + 
            (cpu->address_bus & 0x3FFF)];
        }
    }

}

void mmc1_write(struct Cpu *cpu) {
    if (cpu->address_bus < 0x4020) {
        common_write(cpu);
        return;
    }

    if (cpu->address_bus > 0x7FFF) {
        if ((cpu->data_bus & 128) == 128) {
            mapper->registers[SHIFT_REG] = 0;
            mapper->registers[PRG_MODE] = 3;
            mapper->registers[W_COUNT] = 0;
            return;
        }

        mapper->registers[SHIFT_REG] |= ((cpu->data_bus & 1) << mapper->registers[W_COUNT]);
        
        ++mapper->registers[W_COUNT];
        if (mapper->registers[W_COUNT] == 5) {

            switch(cpu->address_bus & 0x6000) {
                case 0:
                    // Control register
                    switch(mapper->registers[SHIFT_REG] & 3) {
                        case 0:
                            ppu->get_mirrored_addr = one_screen_low_mirroring;
                            break;

                        case 1:
                            ppu->get_mirrored_addr = one_screen_upper_mirroring;
                            break;

                        case 2:
                            ppu->get_mirrored_addr = vertical_mirroring;
                            break;

                        default:
                            ppu->get_mirrored_addr = horizontal_mirroring;
                            break;
                    }
                    mapper->registers[PRG_MODE] = (mapper->registers[SHIFT_REG] >> 2) & 3;
                    mapper->registers[CHR_MODE] = mapper->registers[SHIFT_REG] >> 4;
                    break;

                case 0x2000:
                    // CHR bank 0
                    mapper->registers[CHR_0] = mapper->registers[SHIFT_REG];
                    break;
                    
                case 0x4000:
                    // CHR bank 1
                    mapper->registers[CHR_1] = mapper->registers[SHIFT_REG];
                    break;

                default:
                    // PRG bank
                    mapper->registers[PRG] = mapper->registers[SHIFT_REG] & 0xF;
                    break;
            }

            mapper->registers[SHIFT_REG] = 0;
            mapper->registers[W_COUNT] = 0;
        }

        return;
    }

    if (cpu->address_bus > 0x5FFF) {
        if (mapper->have_prg_ram) {
            mapper->prg_ram[cpu->address_bus & 0x1FFF] = cpu->data_bus;
        }
    }
}

int mmc1_chr_read(int vram_addr) {


    if (mapper->registers[CHR_MODE] == 0) {
        // 8kb mode
        if (mapper->chr_rom_size < 2) {
            return mapper->chr_banks[vram_addr];
        }
        return mapper->chr_banks[(0x2000 * 
                (mapper->registers[CHR_0] >> 1)) + vram_addr];
    }
    // 4kb at a time
    int temp = (vram_addr < 0x1000) ? 
        (0x1000 * (mapper->registers[CHR_0] & mapper->registers[1])) + vram_addr : 
        (0x1000 * (mapper->registers[CHR_1] & mapper->registers[1])) + (vram_addr & 0xFFF);
    return mapper->chr_banks[temp];
}

void mmc1_chr_write(unsigned char data, int vram_addr) {
    
    if (mapper->chr_rom_size == 0) {

       if (mapper->registers[CHR_MODE] == 0) {
           // 8kb mode
           mapper->chr_banks[vram_addr] = data;
       } else {
           // 4kb mode
       int temp = (vram_addr < 0x1000) ? 
            (0x1000 * (mapper->registers[CHR_0] & 1)) + vram_addr : 
            (0x1000 * (mapper->registers[CHR_1] & 1)) + (vram_addr & 0xFFF);
           mapper->chr_banks[temp] = data;
       }

    }

    
}

#undef SHIFT_REG
#undef MIRRORING
#undef PRG_MODE
#undef CHR_MODE
#undef W_COUNT
#undef CHR_0
#undef CHR_1
#undef PRG
