#ifndef _MAPPERS_H
#define _MAPPERS_H

#include "../mos_6502/cpu_impl.h"

typedef struct {
    unsigned char *prg_banks;
    unsigned char *chr_banks;
    unsigned char registers[8];
    void (*vram_read)(struct Cpu *cpu);
    void (*vram_write)(struct Cpu *cpu);
} Mapper;


void common_read(struct Cpu *cpu);
void common_write(struct Cpu *cpu);


void nrom_read(struct Cpu *cpu);
void nrom_write(struct Cpu *cpu);
void nrom_vram_read(struct Cpu *cpu);
void nrom_vram_write(struct Cpu *cpu);

void uxrom_read(struct Cpu *cpu);
void uxrom_write(struct Cpu *cpu);
void uxrom_vram_write(struct Cpu *cpu);
void uxrom_vram_read(struct Cpu *cpu);

extern Mapper *mapper;

#endif
