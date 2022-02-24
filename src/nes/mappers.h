#ifndef _MAPPERS_H
#define _MAPPERS_H

#include "../mos_6502/cpu_impl.h"

typedef struct {
    unsigned char *prg_banks;
    unsigned char *chr_banks;
    unsigned char *prg_ram;
    unsigned char prg_rom_size;
    unsigned char have_prg_ram;
    unsigned char chr_rom_size;
    unsigned char registers[10]; // enough for majority of mappers
    unsigned char irq_counter;
    unsigned char irq_status;
    int (*chr_read)(int vram_add);
    void (*chr_write)(unsigned char data, int vram_addr);
} Mapper;

int vertical_mirroring(int addr);
int horizontal_mirroring(int addr);
int one_screen_low_mirroring(int addr);   // lower bank
int one_screen_upper_mirroring(int addr);   // upper bank

void common_read(struct Cpu *cpu);
void common_write(struct Cpu *cpu);


void nrom_read(struct Cpu *cpu);
void nrom_write(struct Cpu *cpu);
int nrom_chr_read(int vram_addr);
void nrom_chr_write(unsigned char data, int vram_addr);

void uxrom_read(struct Cpu *cpu);
void uxrom_write(struct Cpu *cpu);
int uxrom_chr_read(int vram_addr);
void uxrom_chr_write(unsigned char data, int vram_addr);

void cnrom_read(struct Cpu *cpu);
void cnrom_write(struct Cpu *cpu);
int cnrom_chr_read(int vram_addr);
void cnrom_chr_write(unsigned char data, int vram_addr);


void mmc1_read(struct Cpu *cpu);
void mmc1_write(struct Cpu *cpu);
int mmc1_chr_read(int vram_addr);
void mmc1_chr_write(unsigned char data, int vram_addr);

void mmc3_read(struct Cpu *cpu);
void mmc3_write(struct Cpu *cpu);
int mmc3_chr_read(int vram_addr);
void mmc3_chr_write(unsigned char data, int vram_addr);

void gxrom_read(struct Cpu *cpu);
void gxrom_write(struct Cpu *cpu);
int gxrom_chr_read(int vram_addr);
void gxrom_chr_write(unsigned char data, int vram_addr);

extern Mapper *mapper;

#endif
