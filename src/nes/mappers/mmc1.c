#include "../nes.h"
#include "../mappers.h"

#define SHIFT_REGISTER 0
#define WRITE_COUNTER 1
#define MIRRORING 2
#define CHR_BANK_0 3
#define CHR_BANK_1 4
#define PRG_BANK 5
#define PRG_ROM_BANK_MODE 6
#define CHR_ROM_BANK_MODE 7


void mmc1_read(struct Cpu *cpu) {
    int temp = 0;
    if (cpu->address_bus < 0x4020) {
        common_read(cpu);
        return;
    }

    if (cpu->address_bus > 0xBFFF) {
        // 0xC000 - 0xFFFF
        switch(mapper->registers[PRG_ROM_BANK_MODE]) {
            case 0:
            case 1:
                // Switching 32kb bank
                cpu->data_bus = mapper->prg_banks[(0x8000 * mapper->current_prg_bank) + (cpu->address_bus % 0x8000)];
                printf("32kb bank switch\n");
                break;

            case 2:
                // switch 16 kb bank at 0xC000
                temp = mapper->registers[PRG_BANK] & 0xF;
                if (temp <= mapper->prg_rom_size) {
                    cpu->data_bus = mapper->prg_banks[(0x4000 * temp) + 
                    (cpu->address_bus % 0x4000)];
                } else {
                    printf("16kb prg_banks - %d, 16kb switching - %d\n", 
                            mapper->prg_rom_size + 1, temp);
                    exit(1);
                }
                break;
            default:
                // 3    // fix last bank at 0xC000
                cpu->data_bus = mapper->prg_banks[(0x4000 * mapper->prg_rom_size) + 
                (cpu->address_bus % 0x4000)];
                break;
        }
        return;
    }  
    if (cpu->address_bus > 0x7FFF) {
        // 0x8000 - 0xBFFF
        switch(mapper->registers[PRG_ROM_BANK_MODE]) {
            case 0:
            case 1:
                // Switching 32kb bank
                temp = mapper->registers[PRG_BANK] & 0b1110;
                mapper->current_prg_bank = temp;
                if (temp <= (mapper->prg_rom_size >> 1)) {
                    cpu->data_bus = mapper->prg_banks[(0x8000 * temp) + 
                        (cpu->address_bus % 0x4000)];
                } else {
                    printf("16kb prg_banks - %d, 32kb switching - %d\n", 
                            mapper->prg_rom_size + 1, temp);
                    exit(1);
                }
                break;

            case 2:
                // fix first bank at 0x8000
                cpu->data_bus = mapper->prg_banks[cpu->address_bus % 0x4000];
                break;
            default:
                // 3 switch 16 kb bank at 0x8000
                temp = mapper->registers[PRG_BANK] & 0xF;
                if (temp <= mapper->prg_rom_size) {
                    cpu->data_bus = mapper->prg_banks[(0x4000 * temp) + 
                    (cpu->address_bus % 0x4000)];
                } else {
                    printf("16kb prg_banks - %d, 16kb switching - %d\n", 
                            mapper->prg_rom_size + 1, temp);
                    exit(1);
                }
                break;
        }
        return;
    } 
    if (cpu->address_bus > 0x5FFF) {
        // prg ram
        if (mapper->have_prg_ram) {
            cpu->data_bus = cpu->mem[cpu->address_bus];
        } else {
            printf("Rom don't have prg ram. Reading..\n");
            exit(1);
        }
        return;
    }

    printf("Reading from address 0x%X.\n", cpu->address_bus);
}

void mmc1_write(struct Cpu *cpu) {
    if (cpu->address_bus < 0x4020) {
        common_write(cpu);
        return;
    }

    if (cpu->address_bus > 0x7FFF) {
        if (cpu->data_bus & 0x80) {
            // bit 7 set
            mapper->registers[SHIFT_REGISTER] = 0;   // Clears the shift register
            mapper->registers[WRITE_COUNTER] = 0;
            // locking PRG ROM at $C000-$FFFF to the last bank
            mapper->registers[PRG_ROM_BANK_MODE] = 3;

        } else {
            mapper->registers[SHIFT_REGISTER] |= ((cpu->data_bus & 1) << 
                mapper->registers[WRITE_COUNTER]);
            ++mapper->registers[WRITE_COUNTER];
            if (mapper->registers[WRITE_COUNTER] == 5) {

                // select mapper registers
                switch(cpu->address_bus & 0x6000) {
                    case 0x0:
                        // Control Register
                        switch(mapper->registers[SHIFT_REGISTER] & 3) {
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
                        mapper->registers[PRG_ROM_BANK_MODE] = 
                            (mapper->registers[SHIFT_REGISTER] >> 2) & 3;

                        mapper->registers[CHR_ROM_BANK_MODE] = 
                            (mapper->registers[SHIFT_REGISTER] >> 4);
                        


                        break;

                    case 0x2000:
                        // CHR bank 0
                        mapper->registers[CHR_BANK_0] = mapper->registers[SHIFT_REGISTER];
                        if (mapper->chr_rom_size < 2) {
                            mapper->registers[CHR_BANK_0] &= 1;
                        }
                        
                        break;

                    case 0x4000:
                        // CHR bank 1
                        mapper->registers[CHR_BANK_1] = mapper->registers[SHIFT_REGISTER];
                        if (mapper->chr_rom_size < 2) {
                            mapper->registers[CHR_BANK_1] &= 1;
                        }
                        break;

                    default:
                        // PRG bank
                        mapper->registers[PRG_BANK] = mapper->registers[SHIFT_REGISTER]; break;
                }
                mapper->registers[SHIFT_REGISTER] = 0;
                mapper->registers[WRITE_COUNTER] = 0;
            }
        }
        return;
    }
    if (cpu->address_bus > 0x5FFF) {
        if (mapper->have_prg_ram) {
            cpu->mem[cpu->address_bus] = cpu->data_bus;
        } else {
            printf("Rom don't have prg ram. Writing..\n");
            exit(1);
        }
        return;
    }
    printf("Writing to 0x%X\n", cpu->address_bus);
    /* exit(1); */
}


int mmc1_chr_read(int vram_addr) {

    if (vram_addr < 0x1000) {

        if (mapper->registers[CHR_ROM_BANK_MODE]) {
            // switch 4kb
            return mapper->chr_banks[(0x1000 * mapper->registers[CHR_BANK_0]) + vram_addr];
        } else {
            // switch 8kb
            mapper->current_chr_bank = (mapper->registers[CHR_BANK_0] & 0b11110);
            return mapper->chr_banks[(0x2000 * mapper->current_chr_bank) + vram_addr];
        }
    }

    if (mapper->registers[CHR_ROM_BANK_MODE]) {
        // switch 4kb
        return mapper->chr_banks[(0x1000 * mapper->registers[CHR_BANK_1]) + (vram_addr % 0x1000)];
    } 

    return mapper->chr_banks[(0x2000 * mapper->current_chr_bank) + vram_addr];
}

void mmc1_chr_write(unsigned char data, int vram_addr) {
    if (vram_addr < 0x1000) {

        if (mapper->registers[CHR_ROM_BANK_MODE]) {
            // switch 4kb
            mapper->chr_banks[(0x1000 * mapper->registers[CHR_BANK_0]) + vram_addr] = data;
        } else {
            // switch 8kb
            mapper->current_chr_bank = (mapper->registers[CHR_BANK_0] & 0b11110);
            mapper->chr_banks[(0x2000 * mapper->current_chr_bank) + vram_addr] = data;
        }
        return;
    }

    if (mapper->registers[CHR_ROM_BANK_MODE]) {
        // switch 4kb
       mapper->chr_banks[(0x1000 * mapper->registers[CHR_BANK_1]) + (vram_addr % 0x1000)] = data;
       return;
    } 

    mapper->chr_banks[(0x2000 * mapper->current_chr_bank) + vram_addr] = data;
}



#undef SHIFT_REGISTER
#undef WRITE_COUNTER
#undef MIRRORING
#undef PRG_ROM_BANK_MODE
#undef CHR_ROM_BANK_MODE
#undef CHR_BANK_0
#undef CHR_BANK_1
#undef PRG_BANK
