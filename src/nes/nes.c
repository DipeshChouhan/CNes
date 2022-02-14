// TODO: Remove Cpu memory [...NOT DONE]
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
            // do stuff
            if (prg_rom_size == 1) {
                // 128
                memcpy(&(cpu->mem[0xc000]),data + count, 0x4000);
                /* data += 0x4000; */
                count += 0x4000;
            }else if (prg_rom_size == 2) {
                // 256
                memcpy(&(cpu->mem[0x8000]), data + count,  0x4000);  // first 16kb
                /* data += 0x4000; */
                count += 0x4000;
                memcpy(&(cpu->mem[0xc000]), data + count, 0x4000);  // second 16kb
                /* data += 0x4000; */
                count += 0x4000;

            } else {
                printf("mapper 0 don't support more prg rom than 32kb\n");
                exit(1);
            }
            if (chr_rom_size > 1) {
                printf("mapper 0 don't support more than 8kb of chr rom.\n");
                exit (1);
            }
            if (chr_rom_size == 1) {
                memcpy(&(ppu->ptables), data + count, 0x2000);
            } else {
                // chr ram
                printf("Rom is using CHR RAM.\n");
                /* exit(1); */
            }
            mapper->free_chr_banks = 0;
            mapper->free_prg_banks = 0;
            cpu->cpu_read = nrom_read;
            cpu->cpu_write = nrom_write;
            mapper->vram_read = nrom_vram_read;
            mapper->vram_write = nrom_vram_write;
            break;

        case 1:
            // MMC1
            printf("MMC1 mapper\n");
            if (prg_rom_size < 1) {
                printf("PRG ROM is not available\n");
                exit(1);
            }

            mapper->free_prg_banks = 1;
            mapper->prg_banks = malloc(0x4000 * prg_rom_size);
            memcpy(mapper->prg_banks, data + count, 0x4000 * prg_rom_size);
            count += 0x4000 * prg_rom_size;
            --prg_rom_size;
            mapper->prg_rom_size = prg_rom_size;
            
            if (chr_rom_size == 1) {
                // 8kb chr rom bank
                memcpy(&(ppu->ptables), data + count, 0x2000);
                mapper->free_chr_banks = 0;
                
            } else if (chr_rom_size > 1) {
                mapper->chr_banks = malloc(0x2000 * chr_rom_size);
                memcpy(mapper->chr_banks, data + count, 0x2000 * chr_rom_size);
                mapper->free_chr_banks = 1;
                --chr_rom_size;
            } else {
                printf("Game is using CHR Ram\n");
                mapper->free_chr_banks = 0;
            }
            mapper->chr_rom_size = chr_rom_size;
            cpu->cpu_read = mmc1_read;
            cpu->cpu_write = mmc1_write;
            mapper->vram_read = mmc1_vram_read;
            mapper->vram_write = mmc1_vram_write;
            mapper->registers[0] = 0;  // reset temprory register
            mapper->registers[1] = 0;   // write counter
            mapper->registers[6] = 3;   // fix last bank at 0xC000
            break;

        case 2:
            // uxROM
            printf("UXROM mapper\n");
            --prg_rom_size;
            mapper->prg_rom_size = prg_rom_size;
            mapper->free_chr_banks = 0;
            mapper->free_prg_banks = 1;   // prg rom banks allocated
            // UNROM 7 and UOROM 15
            mapper->registers[1] = (prg_rom_size > 7) ? 15 : 7;
            mapper->prg_banks = malloc(0x4000 * prg_rom_size);
            memcpy(mapper->prg_banks, data + count, 0x4000 * prg_rom_size);
            count += 0x4000 * prg_rom_size;
            memcpy(&(cpu->mem[0xC000]), data + count, 0x4000);
            cpu->cpu_read = uxrom_read;
            cpu->cpu_write = uxrom_write;
            mapper->vram_read = uxrom_vram_read;
            mapper->vram_write = uxrom_vram_write;
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
    if (mapper->free_prg_banks == 1) {
        free(mapper->prg_banks);
    }

    if (mapper->free_chr_banks) {
        free(mapper->chr_banks);
    }
}

