#include "../mappers.h"
#include "../nes.h"

#define BANK_SELECT 8
#define R0 0
#define R1 1
#define R2 2
#define R3 3
#define R4 4
#define R5 5
#define R6 6
#define R7 7

void mmc3_read(struct Cpu *cpu) {

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

    if (cpu->address_bus < 0xA000) {
        // 0x8000 - 0x9FFF
        if (mapper->registers[BANK_SELECT] & 0b01000000) {
            // bit set
            cpu->data_bus = mapper->prg_banks[(0x2000 * (mapper->prg_rom_size - 1)) + 
            (cpu->address_bus & 0x1FFF)];   // second last bank
        } else {
            // bit clear
            cpu->data_bus = mapper->prg_banks[(0x2000 * mapper->registers[R6]) + 
            (cpu->address_bus & 0x1FFF)];
        }

    } else if (cpu->address_bus < 0xC000) {
        // 0xA000 - 0xBFFF
        cpu->data_bus = mapper->prg_banks[(0x2000 * mapper->registers[R7]) + 
        (cpu->address_bus & 0x1FFF)];

    } else if (cpu->address_bus < 0xE000) {
        // 0xC000 - 0xDFFF
        if (mapper->registers[BANK_SELECT] & 0b01000000) {
            // bit set
            cpu->data_bus = mapper->prg_banks[(0x2000 * mapper->registers[R6]) + 
            (cpu->address_bus & 0x1FFF)];
        } else {
            // bit clear
            cpu->data_bus = mapper->prg_banks[(0x2000 * (mapper->prg_rom_size - 1)) + 
            (cpu->address_bus & 0x1FFF)];   // second last bank
        }

    } else {
        // 0xE000 - 0xFFFF
        cpu->data_bus = mapper->prg_banks[(0x2000 * mapper->prg_rom_size) + 
        (cpu->address_bus & 0x1FFF)];
    }

}

void mmc3_write(struct Cpu *cpu) {

    int temp = 0;

    if (cpu->address_bus < 0x4020) {
        common_write(cpu);
        return;
    }

    if (cpu->address_bus < 0x6000) return;

    if (cpu->address_bus < 0x8000 && cpu->address_bus > 0x5FFF) {
        if (mapper->have_prg_ram) {
            mapper->prg_ram[cpu->address_bus & 0x1FFF] = cpu->data_bus;
        }
        return;
    }

    if (cpu->address_bus < 0xA000) {
        // 0x8000 - 0x9FFF
        if (cpu->address_bus & 1) {
            // odd address
            // Bank data
            temp = mapper->registers[BANK_SELECT] & 7;
            if (temp < 2) {
                // R0 and R1 ignore bottom bit
                mapper->registers[temp] = cpu->data_bus & 0b11111110;

            } else if ((mapper->registers[BANK_SELECT] & 7) > 5) {
                // R6 and R7 ignore top two bits
                mapper->registers[temp] = cpu->data_bus & 0b00111111;

            } else {
                mapper->registers[temp] = cpu->data_bus;
            }
        } else {
            // even address 
            // Bank Select
            mapper->registers[BANK_SELECT] = cpu->data_bus;
        }

    } else if (cpu->address_bus < 0xC000) {
        // 0xA000 - 0xBFFF
        if (cpu->address_bus & 1) {
            // odd address
            // PRG RAM protect
        } else {
            // even address
            // TODO: Four Screen Mirroring
            ppu->get_mirrored_addr = (cpu->data_bus & 1) ? horizontal_mirroring :
                vertical_mirroring;
            
        }

    } else if (cpu->address_bus < 0xE000) {
        // 0xC000 - 0xDFFF
        if (cpu->address_bus & 1) {
            // odd address
        } else {
            // even address
        }

    } else {
        // 0xE000 - 0xFFFF
        if (cpu->address_bus & 1) {
            // odd address
        } else {
            // even address
        }

    }
}

void mmc3_chr_write(unsigned char data, int vram_addr) {

    if (mapper->chr_rom_size > 0) return;

    if (mapper->registers[BANK_SELECT] & 0b10000000) {
        // bit set
        if (vram_addr < 0x400) {
            // R2 1KB CHR Bank
            if (mapper->registers[R2] > 7) return;
            vram_addr = (0x400 * mapper->registers[R2]) + vram_addr;
            
        } else if (vram_addr < 0x800) {
            // R3 1KB CHR Bank
            
            if (mapper->registers[R3] > 7) return;
            vram_addr = ((0x400 * mapper->registers[R3]) + vram_addr) & 0x3FF;
        } else if (vram_addr < 0xC00) {
            // R4 1KB CHR Bank
            if (mapper->registers[R4] > 7) return;
            vram_addr = ((0x400 * mapper->registers[R4]) + vram_addr) & 0x3FF;
        } else if (vram_addr < 0x1000) {
            // R5 1KB CHR Bank
            if (mapper->registers[R5] > 7) return;
            vram_addr = ((0x400 * mapper->registers[R5]) + vram_addr) & 0x3FF;
        } else if (vram_addr < 0x1800) {
            // R0 2KB CHR Bank
            if (mapper->registers[R0] > 3) return;
            vram_addr = ((0x800 * mapper->registers[R0]) + vram_addr) & 0x7FF;
        } else {
            // R1 2KB CHR Bank
            if (mapper->registers[R1] > 3) return;
            vram_addr = ((0x800 * mapper->registers[R1]) + vram_addr) & 0x7FF;
        }
    } else {
        // bit clear

        if (vram_addr < 0x800) {
            // R0 2KB CHR Bank
            if (mapper->registers[R0] > 3) return;
            vram_addr = (0x800 * mapper->registers[R0]) + vram_addr;

        } else if (vram_addr < 0x1000) {
            // R1 2KB CHR Bank
            if (mapper->registers[R1] > 3) return;
            vram_addr = ((0x800 * mapper->registers[R1]) + vram_addr) & 0x7FF;
        } else if (vram_addr < 0x1400) {
            // R2 1KB CHR Bank
            if (mapper->registers[R2] > 7) return;
            vram_addr = ((0x400 * mapper->registers[R2]) + vram_addr) & 0x3FF;
        } else if (vram_addr < 0x1800) {
            // R3 1KB CHR Bank
            if (mapper->registers[R3] > 7) return;
            vram_addr = ((0x400 * mapper->registers[R3]) + vram_addr) & 0x3FF;
        } else if (vram_addr < 0x1C00) {
            // R4 1KB CHR Bank
            if (mapper->registers[R4] > 7) return;
            vram_addr = ((0x400 * mapper->registers[R4]) + vram_addr) & 0x3FF;
        } else {
            // R5 1KB CHR Bank
            if (mapper->registers[R5] > 7) return;
            vram_addr = ((0x400 * mapper->registers[R5]) + vram_addr) & 0x3FF;
        }
    }

    mapper->chr_banks[vram_addr] = data;
}

int mmc3_chr_read(int vram_addr){

    if (mapper->registers[BANK_SELECT] & 0b10000000) {
        // bit set
        if (vram_addr < 0x400) {
            // R2 1KB CHR Bank
            vram_addr = (0x400 * mapper->registers[R2]) + vram_addr;
        } else if (vram_addr < 0x800) {
            // R3 1KB CHR Bank
            vram_addr = ((0x400 * mapper->registers[R3]) + vram_addr) & 0x3FF;
        } else if (vram_addr < 0xC00) {
            // R4 1KB CHR Bank
            vram_addr = ((0x400 * mapper->registers[R4]) + vram_addr) & 0x3FF;
        } else if (vram_addr < 0x1000) {
            // R5 1KB CHR Bank
            vram_addr = ((0x400 * mapper->registers[R5]) + vram_addr) & 0x3FF;
        } else if (vram_addr < 0x1800) {
            // R0 2KB CHR Bank
            vram_addr = ((0x800 * mapper->registers[R0]) + vram_addr) & 0x7FF;
        } else {
            // R1 2KB CHR Bank
            vram_addr = ((0x800 * mapper->registers[R1]) + vram_addr) & 0x7FF;
        }
    } else {
        // bit clear

        if (vram_addr < 0x800) {
            // R0 2KB CHR Bank
            vram_addr = (0x800 * mapper->registers[R0]) + vram_addr;
        } else if (vram_addr < 0x1000) {
            // R1 2KB CHR Bank
            vram_addr = ((0x800 * mapper->registers[R1]) + vram_addr) & 0x7FF;
        } else if (vram_addr < 0x1400) {
            // R2 1KB CHR Bank
            vram_addr = ((0x400 * mapper->registers[R2]) + vram_addr) & 0x3FF;
        } else if (vram_addr < 0x1800) {
            // R3 1KB CHR Bank
            vram_addr = ((0x400 * mapper->registers[R3]) + vram_addr) & 0x3FF;
        } else if (vram_addr < 0x1C00) {
            // R4 1KB CHR Bank
            vram_addr = ((0x400 * mapper->registers[R4]) + vram_addr) & 0x3FF;
        } else {
            // R5 1KB CHR Bank
            vram_addr = ((0x400 * mapper->registers[R5]) + vram_addr) & 0x3FF;
        }
    }
    return mapper->chr_banks[vram_addr];
}


