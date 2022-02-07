#ifndef CPU_IMPL_HEADER
#define CPU_IMPL_HEADER
#include <stdlib.h>
#include<stdint.h>


struct flag_bits{
    uint8_t c : 1;
    uint8_t z : 1;
    uint8_t i : 1;
    uint8_t d : 1;
    uint8_t b : 1;
    uint8_t nu : 1;     // not used
    uint8_t v : 1;
    uint8_t n : 1;
};
union flags {
    uint8_t value;
    struct flag_bits bits;
};

// mos 6502
 struct Cpu{
    uint8_t mem[65536];     // 64kb memory   
    int cycles;             // cpu cycles
    uint16_t pc;            // program counter
    uint8_t sp;             // stack pointer
    union flags ps;             // flags
    uint8_t a;              // accumulator
    uint8_t x;              // index register x
    uint8_t y;              // index register y

    uint8_t ir;             // instruction register
    uint8_t data_bus;       // bi-directional bus
    uint16_t address_bus;   // uni-directional bus

    uint8_t rdy;            // ready pin
    uint8_t res;            // res pin
    uint8_t nmi;            // nmi pin
    uint8_t irq;            // irq pin
    uint8_t rw;             // read write pin
    uint8_t sync;           // sync pin
    void (*cpu_read)(struct Cpu *cpu);
    void (*cpu_write)(struct Cpu *cpu);
};

unsigned int cpu_cycle(struct Cpu *cpu);
void power_on(struct Cpu *cpu);
void initialize(struct Cpu *cpu);       // initialize default read and write

#endif
