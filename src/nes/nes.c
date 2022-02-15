#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "nes.h"
#include "../utils.h"
#include "mappers.h"


#define PRG_ROM_SIZE 4
#define CHR_ROM_SIZE 5

Ppu *ppu;
SDL_Event event;
Mapper *mapper;

int cpu_oamdma_addr = 0;
int oamdma_addr = 0;


void init_mapper(int prg_rom_size, int chr_rom_size, unsigned char *data) {
    int count = 16;

    mapper->prg_banks = malloc(0x4000 * prg_rom_size);
    memcpy(mapper->prg_banks, data + count, 0x4000 * prg_rom_size);
    count += 0x4000 * prg_rom_size;
    if (chr_rom_size == 0) {
        mapper->chr_banks = malloc(0x2000);
    } else {
        mapper->chr_banks = malloc(0x2000 * chr_rom_size);
        memcpy(mapper->chr_banks, data + count, 0x2000 * chr_rom_size);
    }
}

void loadMapper(char *game_file, struct Cpu *cpu) {
    unsigned int size; unsigned char *data = load_binary_file(game_file, &size);

    printf("file_size: %d\n", size); int prg_rom_size = data[PRG_ROM_SIZE];
    int chr_rom_size = data[CHR_ROM_SIZE];


    // flag6
    /* 76543210 */
    /* |||||||| */
    /* |||||||+- Mirroring: 0: horizontal (vertical arrangement) (CIRAM A10 = PPU A11) */
    /* |||||||              1: vertical (horizontal arrangement) (CIRAM A10 = PPU A10) */
    /* ||||||+-- 1: Cartridge contains battery-backed PRG RAM ($6000-7FFF) or other persistent memory */
    /* |||||+--- 1: 512-byte trainer at $7000-$71FF (stored before PRG data) */
    /* ||||+---- 1: Ignore mirroring control or above mirroring bit; instead provide four-screen VRAM */
    /* ++++----- Lower nybble of mapper number */

    int flag6 = data[6];

    /* 76543210 */
    /* |||||||| */
    /* |||||||+- vs unisystem */
    /* ||||||+-- playchoice-10 (8kb of hint screen data stored after chr data) */
    /* ||||++--- if equal to 2, flags 8-15 are in nes 2.0 format */
    /* ++++----- upper nybble of mapper number */

    int flag7 = data[7];

    int mirroring = flag6 & 1;
    int mapper_type = (flag7 & 0xf0) | (flag6 >> 4);
    int trainer = (flag6 >> 2) & 1;
    mapper->have_prg_ram = (flag6 >> 1) & 1;
    int four_screen_mirroring = (flag6 >> 3) & 1;

    if ((flag7 & 0b00001100) == 0b00001000) {
        printf("Rom is in NES2.0 fromat. Not Supported\n");
        exit(1);
    }

    if (!four_screen_mirroring) {
        if (flag6 & 1) {
            ppu->get_mirrored_addr = vertical_mirroring;
        } else {
            ppu->get_mirrored_addr = horizontal_mirroring;
        }
    } else {
        // TODO: provide four screen mirroring
        printf("Game using four screen mirroring\n");
        exit(1);
    }

    printf("mapper = %d, mirroring = %d, trainer = %d, prg_rom = %d, chr_rom = %d, prg_ram = %d\n",
            mapper_type, mirroring, trainer, prg_rom_size*0x4000, chr_rom_size * 0x2000, mapper->have_prg_ram);

    int count = 16;

    switch (mapper_type) {
        // mapper nrom
        case 0:
            init_mapper(prg_rom_size, chr_rom_size, data);
            mapper->prg_rom_size = prg_rom_size;
            mapper->chr_rom_size = chr_rom_size;
            cpu->cpu_read = nrom_read;
            cpu->cpu_write = nrom_write;
            mapper->chr_read = nrom_chr_read;
            mapper->chr_write = nrom_chr_write;
            break;

        case 1:
            // MMC1
            printf("MMC1 mapper\n");

            if (mapper->have_prg_ram) {
                mapper->prg_ram = malloc(0x2000);
            }
            init_mapper(prg_rom_size, chr_rom_size, data);
            mapper->prg_rom_size = prg_rom_size - 1;
            
            mapper->chr_rom_size = chr_rom_size;
            cpu->cpu_read = mmc1_read;
            cpu->cpu_write = mmc1_write;
            mapper->chr_read = mmc1_chr_read;
            mapper->chr_write = mmc1_chr_write;
            mapper->registers[0] = 0;  // reset temprory register
            mapper->registers[7] = 0;   // write counter
            mapper->registers[1] = (chr_rom_size < 2) ? 1 : 0x1F;
            mapper->registers[2] = 3;   // fix last bank at 0xC000
            break;

        case 2:
            // uxROM
            printf("UXROM mapper\n");
            mapper->prg_rom_size = prg_rom_size - 1;
            init_mapper(prg_rom_size, chr_rom_size, data);
            mapper->registers[1] = prg_rom_size > 8 ? 15 : 7;
            mapper->chr_rom_size = chr_rom_size;
            cpu->cpu_read = uxrom_read;
            cpu->cpu_write = uxrom_write;
            mapper->chr_read = uxrom_chr_read;
            mapper->chr_write = uxrom_chr_write;
            break;

        case 3:
            printf("CNROM mapper\n");
            mapper->chr_rom_size = chr_rom_size;
            mapper->prg_rom_size = prg_rom_size;
            init_mapper(prg_rom_size, chr_rom_size, data);
            cpu->cpu_read = cnrom_read;
            cpu->cpu_write = cnrom_write;
            mapper->chr_read = cnrom_chr_read;
            mapper->chr_write = cnrom_chr_write;
            break;


        case 66:
            // gxrom
            printf("GXROM mapper\n");
            mapper->prg_rom_size = prg_rom_size;
            mapper->chr_rom_size = chr_rom_size;
            init_mapper(prg_rom_size, chr_rom_size, data);
            cpu->cpu_read = gxrom_read;
            cpu->cpu_write = gxrom_write;
            mapper->chr_read = gxrom_chr_read;
            mapper->chr_write = gxrom_chr_write;
            break;

        default:
            printf("mapper %d is not supported\n", mapper_type);
            exit(1);
                
    }

    free(data);

    // identify mapper and load file into cpu
}


void power_on_nes(char *game_file) {
    struct Cpu cpu;
    Ppu p;
    p.total_sprites = 0;
    p.total_cycles = 0;
    p.oam_dma = 0;
    p.write_toggle = 0;
    p.ppu_status = 0;
    p.ppu_ctrl = 0;
    p.ppu_mask = 0;
    ppu = &p;
    Mapper m;
    mapper = &m;
    loadMapper(game_file, &cpu);
    power_on(&cpu);
    ppu_render(&cpu);
    free(mapper->prg_banks);

    free(mapper->chr_banks);

    if (mapper->have_prg_ram) {
        free(mapper->prg_ram);
    }
}

