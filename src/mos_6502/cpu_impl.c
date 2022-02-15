#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "cpu_impl.h"

/* #define DEBUG_ON */
#define PRINT(_cpu)                                                 \
    printf("%0X     , A: %0X    , X: %0X    , Y: %0X    , PS: %0X   , SP: %0X, cycles - %d\n", _cpu->pc, _cpu->a, _cpu->x, _cpu->y, _cpu->ps.value, _cpu->sp, cpu->total_cycles); \


#define cycles cpu->cycles


    /* _cpu->ir = _cpu->mem[_cpu->pc];         \ */
#define FETCH_OPCODE(_cpu)                  \
    _cpu->sync = 1;                         \
    _cpu->address_bus = _cpu->pc;            \
    _cpu->cpu_read(_cpu);                   \
    _cpu->ir = _cpu->data_bus;              \
    _cpu->pc += 1;                          \
    _cpu->sync = 0;                         \

#define PAGE_CROSSED                        \
    if (temp>>8 != cpu->address_bus>>8) {   \
        ++cycles;                           \
    }                                       \

#define PUSH_STACK(_cpu, _val)              \
    _cpu->rw = 0;                           \
    _cpu->mem[(0x1<<8)|_cpu->sp] = _val;    \
    _cpu->sp -= 1;                          \

#define PULL_STACK(_cpu)                    \
    _cpu->sp += 1;                          \
    _cpu->data_bus = _cpu->mem[(0x1<<8)|_cpu->sp];  \

#define FETCH_ADL(_cpu)                     \
    _cpu->address_bus = _cpu->pc;           \
    _cpu->cpu_read(_cpu);                   \
    adl = _cpu->data_bus;                   \
    _cpu->pc += 1;                          \

#define FETCH_BAL(_cpu)                     \
    _cpu->address_bus = _cpu->pc;           \
    _cpu->cpu_read(_cpu);                   \
    bal = _cpu->data_bus;                   \
    _cpu->pc += 1;                          \

#define FETCH_ADH(_cpu)                     \
    _cpu->address_bus = _cpu->pc;           \
    _cpu->cpu_read(_cpu);                   \
    adh = _cpu->data_bus;                   \
    _cpu->pc += 1;                          \

#define FETCH_ADDR(_cpu, _addr)             \
    _cpu->address_bus = _addr;              \
    _cpu->cpu_read(_cpu);                   \

#define IMMEDIATE(_cpu)                     \
    FETCH_ADDR(_cpu, _cpu->pc);             \
    _cpu->pc += 1;                          \

// One additional cycle if page crossed
#define RELATIVE(_cpu, _cond)               \
    FETCH_ADDR(_cpu, _cpu->pc);             \
    _cpu->pc += 1;                          \
    if(_cond){                              \
        adh = _cpu->pc>>8;                  \
        _cpu->pc += (int8_t)_cpu->data_bus; \
        cycles = 3;                         \
        if(adh != (_cpu->pc>>8)){           \
            cycles = 4;                     \
        }                                   \
    }                                       \


#define ABSOLUTE(_cpu)                      \
    FETCH_ADL(_cpu);                        \
    FETCH_ADH(_cpu);                        \
    FETCH_ADDR(_cpu, (adh<<8)|adl);         \

    /* _cpu->mem[(adh<<8)|adl] = _val;         \ */
#define W_ABSOLUTE(_cpu, _val)              \
    FETCH_ADL(_cpu);                        \
    FETCH_ADH(_cpu);                        \
    _cpu->rw = 0;                           \
    _cpu->data_bus = _val;                  \
    _cpu->address_bus = (adh<<8)|adl;       \
    _cpu->cpu_write(_cpu);                  \
    cycles = 4;                             \
    

#define ZERO_PAGE(_cpu)                     \
    FETCH_BAL(_cpu);                        \
    _cpu->data_bus = _cpu->mem[bal];        \

#define W_ZERO_PAGE(_cpu, _val)             \
    FETCH_BAL(_cpu);                        \
    _cpu->rw = 0;                           \
    _cpu->mem[bal] = _val;                  \
    cycles = 3;                             \


#define ZERO_PAGE_X(_cpu)                           \
    FETCH_BAL(_cpu);                                \
    bal =(uint8_t)(bal + _cpu->x);                                 \
    _cpu->data_bus = _cpu->mem[bal];      \

#define W_ZERO_PAGE_X(_cpu, _val)                   \
    FETCH_BAL(_cpu);                                \
    _cpu->rw = 0;                                   \
    _cpu->mem[(uint8_t)(bal + _cpu->x)] = _val;     \
    cycles = 4;                                     \

// Only with LDX, STX
#define ZERO_PAGE_Y(_cpu)                           \
    FETCH_BAL(_cpu);                                \
    bal = (uint8_t)(bal + _cpu->y);                 \
    _cpu->data_bus = _cpu->mem[bal];      \

#define W_ZERO_PAGE_Y(_cpu, _val)                   \
    FETCH_BAL(_cpu);                                \
    _cpu->rw = 0;                                   \
    _cpu->mem[(uint8_t)(bal + _cpu->y)] = _val;     \
    cycles = 4;                                     \


#define ABSOLUTE_X(_cpu)                            \
    FETCH_ADL(_cpu);                                \
    FETCH_ADH(_cpu);                                \
    _cpu->address_bus = (adh<<8)|adl;               \
    temp = _cpu->address_bus;                        \
    _cpu->address_bus += _cpu->x;                   \
    _cpu->cpu_read(_cpu);                           \

#define W_ABSOLUTE_X(_cpu, _val)                    \
    FETCH_ADL(_cpu);                                \
    FETCH_ADH(_cpu);                                \
    _cpu->rw = 0;                                   \
    _cpu->address_bus = (adh<<8)|adl;               \
    _cpu->address_bus += _cpu->x;                   \
    _cpu->data_bus = _val;                          \
    cycles = 5;                                     \
    _cpu->cpu_write(_cpu);                          \

#define ABSOLUTE_Y(_cpu)                            \
    FETCH_ADL(_cpu);                                \
    FETCH_ADH(_cpu);                                \
    _cpu->address_bus = (adh<<8)|adl;               \
    temp = _cpu->address_bus;                       \
    _cpu->address_bus += _cpu->y;                   \
    _cpu->cpu_read(_cpu);                           \

#define W_ABSOLUTE_Y(_cpu, _val)                    \
    FETCH_ADL(_cpu);                                \
    FETCH_ADH(_cpu);                                \
    _cpu->rw = 0;                                   \
    _cpu->address_bus = (adh<<8)|adl;               \
    _cpu->address_bus += _cpu->y;                   \
    _cpu->data_bus = _val;                          \
    cycles = 5;                                     \
    _cpu->cpu_write(_cpu);                          \

#define INDEXED_INDIRECT(_cpu)                          \
    FETCH_BAL(_cpu);                                    \
    bal += _cpu->x;                                     \
    adl = _cpu->mem[(uint8_t)bal];           \
    adh = _cpu->mem[(uint8_t)(bal + 1)];     \
    FETCH_ADDR(_cpu, (adh<<8)|adl);                     \

#define W_INDEXED_INDIRECT(_cpu, _val)                  \
    FETCH_BAL(_cpu);                                    \
    bal += _cpu->x;                                     \
    adl = _cpu->mem[(uint8_t)bal];           \
    adh = _cpu->mem[(uint8_t)(bal + 1)];     \
    _cpu->rw = 0;                                       \
    _cpu->address_bus = (adh<<8)|adl;                   \
    _cpu->data_bus = _val;                              \
    cycles = 6;                                     \
    _cpu->cpu_write(_cpu);                              \

#define INDIRECT_INDEXED(_cpu)                          \
    FETCH_BAL(_cpu);                                    \
    adl = _cpu->mem[bal];                    \
    adh = _cpu->mem[(uint8_t)(bal + 1)];               \
    _cpu->address_bus = (adh << 8)|adl;                 \
    temp = _cpu->address_bus;                           \
    _cpu->address_bus += _cpu->y;                       \
    _cpu->cpu_read(_cpu);                               \

    
#define W_INDIRECT_INDEXED(_cpu, _val)                  \
    FETCH_BAL(_cpu);                                    \
    adl = _cpu->mem[bal];                    \
    adh = _cpu->mem[(uint8_t)(bal + 1)];               \
    _cpu->rw = 0;                                       \
    _cpu->address_bus = (adh<<8)|adl;                   \
    _cpu->address_bus += _cpu->y;                       \
    _cpu->data_bus = _val;                              \
    cycles = 6;                                     \
    _cpu->cpu_write(_cpu);                              \


#define SHIFT_LEFT(_data)                               \
            CF = _data>>7;                              \
            _data = _data << 1;                         \
            ZF_NF(_data);                               \

#define SHIFT_RIGHT(_data)                              \
            CF = _data & 1;                             \
            _data = _data >> 1;                         \
            NF = 0;                                     \
            ZF = _data == 0;                            \

#define ROTAT_LEFT_FL(_data)                            \
            temp = CF;                                  \
            CF = _data>>7;                              \
            _data = (_data << 1)|temp;                  \

#define ROTATE_LEFT(_data)                              \
            temp = CF;                                  \
            CF = _data>>7;                              \
            _data = (_data << 1)|temp;                  \
            ZF_NF(_data);                               \

#define ROTATE_RIGHT_FL(_data)                          \
            temp = CF;                                  \
            CF = _data & 1;                             \
            _data = (_data >> 1) | (temp << 7);         \

#define ROTATE_RIGHT(_data)                             \
            temp = CF;                                  \
            NF = CF;                                    \
            CF = _data & 1;                             \
            _data = (_data >> 1) | (temp << 7);         \
            ZF = _data == 0;                            \

#define RESET_VECTOR_LOW 0xFFFC
#define RESET_VECTOR_HIGH 0xFFFD

#define IRQ_VECTOR_LOW 0xFFFE
#define IRQ_VECTOR_HIGH 0xFFFF

#define NMI_VECTOR_LOW 0xFFFA
#define NMI_VECTOR_HIGH 0xFFFB

#define DATA cpu->data_bus
#define SP cpu->sp
#define A cpu->a
#define X cpu->x
#define Y cpu->y
#define PC cpu->pc
#define PS cpu->ps

#define ZF cpu->ps.bits.z
#define NF cpu->ps.bits.n
#define CF cpu->ps.bits.c
#define VF cpu->ps.bits.v
#define DF cpu->ps.bits.d
#define BF cpu->ps.bits.b
#define IDF cpu->ps.bits.i
#define NUF cpu->ps.bits.nu

#define ZF_NF(_v)               \
    ZF = _v == 0;               \
    NF = _v>>7;                 \

void power_on(struct Cpu *cpu) {
    // Hold during reset    // address bus contains garbage // T0
    // +1 // First start state          // T1
    // 0x100 + SP   // Second start state       // T2
    // 0x100 + SP - 1   // Third start state    // T3
    // 0x100 + SP - 2   // Fourth start state   // T4
    // FETCH first vector
    IDF = 1;        // Turn on interrupt disable bit
    /* BF = 1;   // for nestest */
    NUF = 1;
    cpu->sp = 0xFD;
    cpu->nmi = 0;
    cpu->address_bus = RESET_VECTOR_LOW;
    cpu->cpu_read(cpu);
    cpu->pc = cpu->data_bus;
    // FETCH second vector
    cpu->address_bus = RESET_VECTOR_HIGH;
    cpu->cpu_read(cpu);
    cpu->pc = (cpu->data_bus << 8) | cpu->pc;   // T6   PCH PCL
    cpu->total_cycles = 7;
    printf("pc vector %0X\n", cpu->pc);

    /* printf("power on %0x\n", cpu->pc); */
    // now cpu is initialized to load first op code in 7 cycles
}

unsigned int cpu_cycle(struct Cpu *cpu) {

    cycles = 2;
    int temp = 0, bal = 0, adh = 0, adl = 0;       // traps to hold addresses and results

    if(cpu->nmi) {
        // process interrupt nmi
        /* cpu->pc += 1; */
        PUSH_STACK(cpu, (cpu->pc >> 8));    // T1, T2 PCH
        PUSH_STACK(cpu, (cpu->pc & 255));   // T3 PCL
        PUSH_STACK(cpu, PS.value);    // T4
        cpu->rw = 1;
        FETCH_ADDR(cpu, NMI_VECTOR_LOW);  // T5
        adl = cpu->data_bus;
        FETCH_ADDR(cpu, NMI_VECTOR_HIGH); // T6
        IDF = 1;    
        adh = cpu->data_bus;
        cpu->pc = (adh << 8)|adl;
        cpu->nmi = 0;
        cpu->total_cycles += 7;
        return 7;
    }
    
#ifdef DEBUG_ON
    /* char input[10]; */
    PRINT(cpu);
    /* scanf("%s", input); */
#endif
    
    FETCH_OPCODE(cpu);

    switch(cpu->ir) {
        // LDA
        case 0xA9:
            // immediate
            /* printf("LDA Immediate  A - %0X\n", A); */
            IMMEDIATE(cpu);
            A = DATA;
            ZF_NF(A);
            break;
            
        case 0xA5:
            // Zero Page
            /* printf("LDA Zero Page\n"); */
            ZERO_PAGE(cpu);
            A = DATA;
            ZF_NF(A);
            cycles = 3;
            break;

        case 0xB5:
            // Zero Page X
            /* printf("LDA Zero Page X\n"); */
            ZERO_PAGE_X(cpu);
            A = DATA;
            /* printf("A - %0X\n", A); */
            ZF_NF(A);
            cycles = 4;
            break;

        case 0xAD:
            // Absolute
            /* printf("LDA Absolute\n"); */
            ABSOLUTE(cpu);
            A = DATA;
            ZF_NF(A);
            cycles = 4;
            break;

        case 0xBD:
            // Absolute X
            /* printf("LDA Absolute X\n"); */
            ABSOLUTE_X(cpu);
            cycles = 4;
            PAGE_CROSSED;
            A = DATA;
            /* printf(" X - %0X, A - %0X\n",X, cpu->mem[(uint16_t)((adh<<8)|(adl + X))]); */
            ZF_NF(A);
            break;

        case 0xB9:
            // Absolute Y
            /* printf("LDA Absolute Y\n"); */
            ABSOLUTE_Y(cpu);
            cycles = 4;
            PAGE_CROSSED;
            A = DATA;
            ZF_NF(A);
            break;

        case 0xA1:
            // Indexed Indirect
            /* printf("LDA Indexed Indirect\n"); */
            INDEXED_INDIRECT(cpu);
            A = DATA;
            ZF_NF(A);
            cycles = 6;
            break;
            
        case 0xB1:
            // Indirect indexed addressing
            /* printf("LDA Indirect Indexed\n"); */
            INDIRECT_INDEXED(cpu);
            cycles = 5;
            PAGE_CROSSED;
            A = DATA;
            ZF_NF(A);
            break;

        // LAX Unoffical Opcode
        
        case 0xA7:
            // Zero Page LAX
            ZERO_PAGE(cpu);
            A = DATA;
            X = DATA;
            cycles = 3;
            ZF_NF(A);
            break;

        case 0xB7:
            // Zero Page Y LAX
            ZERO_PAGE_Y(cpu);
            A = DATA;
            X = DATA;
            cycles = 4;
            ZF_NF(A);
            break;

        case 0xAF:
            // Absolute LAX
            ABSOLUTE(cpu);
            A = DATA;
            X = DATA;
            cycles = 4;
            ZF_NF(A);
            break;

        case 0xBF:
            // Absolute Y LAX
            ABSOLUTE_Y(cpu);
            A = DATA;
            X = DATA;
            cycles = 4;
            PAGE_CROSSED;
            ZF_NF(A);
            break;

        case 0xA3:
            // Indexed Indirect LAX
            INDEXED_INDIRECT(cpu);
            A = DATA;
            X = DATA;
            cycles = 6;
            ZF_NF(A);
            break;

        case 0xB3:
            // Indirect Indexed LAX
            INDIRECT_INDEXED(cpu);
            A = DATA;
            X = DATA;
            cycles = 5;
            PAGE_CROSSED;
            ZF_NF(A);
            break;

        // STA
        case 0x85:
            // Zero Page
            /* printf("STA Zero Page\n"); */
            W_ZERO_PAGE(cpu, A);
            cpu->rw = 1;
            break;
            
        case 0x95:
            // Zero Page X
            /* printf("STA Zero Page X\n"); */
            W_ZERO_PAGE_X(cpu, A);
            cpu->rw = 1;
            break;

        case 0x8D:
            // Absolute
            /* printf("STA Absolute\n"); */
            W_ABSOLUTE(cpu, A);
            cpu->rw = 1;
            break;

        case 0x9D:
            // Absolute X
            /* printf("STA Absolute X\n"); */
            W_ABSOLUTE_X(cpu, A);
            cpu->rw = 1;
            break;

        case 0x99:
            // Absolute Y
            /* printf("STA Absolute Y\n"); */
            W_ABSOLUTE_Y(cpu, A);
            cpu->rw = 1;
            break;

        case 0x81:
            // Indexed Indirect
            /* printf("STA Indexed Indirect\n"); */
            W_INDEXED_INDIRECT(cpu, A);
            cpu->rw = 1;
            break;
        
        case 0x91:
            // Indirect Indexed
            /* printf("STA Indirect Indexed\n"); */
            W_INDIRECT_INDEXED(cpu, A);
            cpu->rw = 1;
            break;

        // SAX Unoffical Opcode
        
        case 0x83:
            // Indexed Indirect SAX
            W_INDEXED_INDIRECT(cpu, (A & X));
            cpu->rw = 1;
            break;

        case 0x87:
            // Zero Page SAX
            W_ZERO_PAGE(cpu, (A & X));
            cpu->rw = 1;
            break;

        case 0x8F:
            // Absolute SAX
            W_ABSOLUTE(cpu, (A & X));
            cpu->rw = 1;
            break;

        case 0x97:
            // Zero Page Y SAX
            W_ZERO_PAGE_Y(cpu, (A & X));
            cpu->rw = 1;
            break;



        // ADC
        case 0x69:
            /* printf("ADC Immediate\n"); */
            IMMEDIATE(cpu);
            temp = A + DATA + CF;
            CF = temp>>8;   // carry bit
            VF = ((A>>7) == (DATA>>7)) && (((temp>>7)&1) != A>>7);
            A = temp;
            ZF_NF(A);
            break;
            
        case 0x65:
            /* printf("ADC Zero Page\n"); */
            ZERO_PAGE(cpu);
            temp = A + DATA + CF;
            CF = temp>>8;   // carry bit
            VF = ((A>>7) == (DATA>>7)) && (((temp>>7)&1) != A>>7);
            A = temp;
            ZF_NF(A);
            cycles = 3;
            break;

        case 0x75:
            /* printf("ADC Zero Page, X\n"); */
            ZERO_PAGE_X(cpu);
            temp = A + DATA + CF;
            CF = temp>>8;   // carry bit
            VF = ((A>>7) == (DATA>>7)) && (((temp>>7)&1) != A>>7);
            A = temp;
            ZF_NF(A);
            cycles = 4;
            break;

        case 0x6D:
            /* printf("ADC Absolute\n"); */
            ABSOLUTE(cpu);
            temp = A + DATA + CF;
            CF = temp>>8;   // carry bit
            VF = ((A>>7) == (DATA>>7)) && (((temp>>7)&1) != A>>7);
            A = temp;
            ZF_NF(A);
            cycles = 4;
            break;

        case 0x7D:
            /* printf("ADC Absolute, X\n"); */
            ABSOLUTE_X(cpu);
            cycles = 4;
            PAGE_CROSSED;
            temp = A + DATA + CF;
            CF = temp>>8;   // carry bit
            VF = ((A>>7) == (DATA>>7)) && (((temp>>7)&1) != A>>7);
            A = temp;
            ZF_NF(A);
            break;

        case 0x79:
            /* printf("ADC Absolute, Y\n"); */
            ABSOLUTE_Y(cpu);
            cycles = 4;
            PAGE_CROSSED;
            temp = A + DATA + CF;
            CF = temp>>8;   // carry bit
            VF = ((A>>7) == (DATA>>7)) && (((temp>>7)&1) != A>>7);
            A = temp;
            ZF_NF(A);
            break;
            
        case 0x61:
            /* printf("ADC Indexed Indirect\n"); */
            INDEXED_INDIRECT(cpu);
            temp = A + DATA + CF;
            CF = temp>>8;   // carry bit
            VF = ((A>>7) == (DATA>>7)) && (((temp>>7)&1) != A>>7);
            A = temp;
            ZF_NF(A);
            cycles = 6;
            break;
            
        case 0x71:
            /* printf("ADC Indirect Indexed\n"); */
            INDIRECT_INDEXED(cpu);
            cycles = 5;
            PAGE_CROSSED;
            temp = A + DATA + CF;
            CF = temp>>8;   // carry bit
            VF = ((A>>7) == (DATA>>7)) && (((temp>>7)&1) != A>>7);
            A = temp;
            ZF_NF(A);
            break;

        // ISB Unoffical opcode
        case 0xE3:
            // Indexed Indirect ISB
            INDEXED_INDIRECT(cpu);
            DATA += 1;
            cpu->cpu_write(cpu);
            // Taking two's complement of data
            temp = A + ((uint8_t)(~DATA) + CF);
            VF = ((A>>7) == (((~DATA + CF)>>7)&1)) && (((temp>>7)&1) != A>>7);
            CF = temp>>8;       // borrow
            A = temp;
            ZF_NF(A);
            cycles = 8;
            break;

        case 0xE7:
            // Zero Page ISB
            ZERO_PAGE(cpu);
            DATA += 1;
            cpu->mem[bal] = DATA;
            // Taking two's complement of data
            temp = A + ((uint8_t)(~DATA) + CF);
            VF = ((A>>7) == (((~DATA + CF)>>7)&1)) && (((temp>>7)&1) != A>>7);
            CF = temp>>8;       // borrow
            A = temp;
            ZF_NF(A);
            cycles = 5;
            break;

        case 0xF7:
            // Zero Page, X ISB
            ZERO_PAGE_X(cpu);
            DATA += 1;
            cpu->mem[(uint8_t)bal] = DATA;
            // Taking two's complement of data
            temp = A + ((uint8_t)(~DATA) + CF);
            VF = ((A>>7) == (((~DATA + CF)>>7)&1)) && (((temp>>7)&1) != A>>7);
            CF = temp>>8;       // borrow
            A = temp;
            ZF_NF(A);
            cycles = 6;
            break;

        case 0xEF:
            // ABSOLUTE ISB
            ABSOLUTE(cpu);
            DATA += 1;
            cpu->cpu_write(cpu);
            // Taking two's complement of data
            temp = A + ((uint8_t)(~DATA) + CF);
            VF = ((A>>7) == (((~DATA + CF)>>7)&1)) && (((temp>>7)&1) != A>>7);
            CF = temp>>8;       // borrow
            A = temp;
            ZF_NF(A);
            cycles = 6;
            break;

        case 0xFF:
            // ABSOLUTE_X ISB
            ABSOLUTE_X(cpu);
            DATA += 1;
            cpu->cpu_write(cpu);
            // Taking two's complement of data
            temp = A + ((uint8_t)(~DATA) + CF);
            VF = ((A>>7) == (((~DATA + CF)>>7)&1)) && (((temp>>7)&1) != A>>7);
            CF = temp>>8;       // borrow
            A = temp;
            ZF_NF(A);
            cycles = 7;
            break;

        case 0xFB:
            // ABSOLUTE_Y ISB
            ABSOLUTE_Y(cpu);
            DATA += 1;
            cpu->cpu_write(cpu);
            // Taking two's complement of data
            temp = A + ((uint8_t)(~DATA) + CF);
            VF = ((A>>7) == (((~DATA + CF)>>7)&1)) && (((temp>>7)&1) != A>>7);
            CF = temp>>8;       // borrow
            A = temp;
            ZF_NF(A);
            cycles = 7;
            break;

        case 0xF3:
            // INDIRECT_INDEXED ISB
            INDIRECT_INDEXED(cpu);
            DATA += 1;
            cpu->cpu_write(cpu);
            // Taking two's complement of data
            temp = A + ((uint8_t)(~DATA) + CF);
            VF = ((A>>7) == (((~DATA + CF)>>7)&1)) && (((temp>>7)&1) != A>>7);
            CF = temp>>8;       // borrow
            A = temp;
            ZF_NF(A);
            cycles = 8;
            break;

        // SBC
        // REMINDER: If any error occured. Check SBC implementation and specially setting of VF and CF flags
        case 0xE9:
            /* printf("SBC Immediate\n"); */
            IMMEDIATE(cpu);
            // Taking two's complement of data
            temp = A + ((uint8_t)(~DATA) + CF);
            VF = ((A>>7) == (((~DATA + CF)>>7)&1)) && (((temp>>7)&1) != A>>7);
            CF = temp>>8;       // borrow
            A = temp;
            ZF_NF(A);
            break;

        case 0xEB:
            // Uofficial same as 0xE9
            IMMEDIATE(cpu);
            // Taking two's complement of data
            temp = A + ((uint8_t)(~DATA) + CF);
            VF = ((A>>7) == (((~DATA + CF)>>7)&1)) && (((temp>>7)&1) != A>>7);
            CF = temp>>8;       // borrow
            A = temp;
            ZF_NF(A);
            break;
            
        case 0xE5:
            /* printf("SBC Zero Page\n"); */
            ZERO_PAGE(cpu);
            // Taking two's complement of data
            temp = A + ((uint8_t)(~DATA) + CF);
            VF = ((A>>7) == (((~DATA + CF)>>7)&1)) && (((temp>>7)&1) != A>>7);
            CF = temp>>8;       // borrow
            A = temp;
            ZF_NF(A);
            cycles = 3;
            break;

        case 0xF5:
            /* printf("SBC Zero Page, X\n"); */
            ZERO_PAGE_X(cpu);
            // Taking two's complement of data
            temp = A + ((uint8_t)(~DATA) + CF);
            VF = ((A>>7) == (((~DATA + CF)>>7)&1)) && (((temp>>7)&1) != A>>7);
            CF = temp>>8;       // borrow
            A = temp;
            ZF_NF(A);
            cycles = 4;
            break;

        case 0xED:
            /* printf("SBC Absolute\n"); */
            ABSOLUTE(cpu);
            // Taking two's complement of data
            temp = A + ((uint8_t)(~DATA) + CF);
            VF = ((A>>7) == (((~DATA + CF)>>7)&1)) && (((temp>>7)&1) != A>>7);
            CF = temp>>8;       // borrow
            A = temp;
            ZF_NF(A);
            cycles = 4;
            break;

        case 0xFD:
            /* printf("SBC Absolute, X\n"); */
            ABSOLUTE_X(cpu);
            cycles = 4;
            PAGE_CROSSED;
            // Taking two's complement of data
            temp = A + ((uint8_t)(~DATA) + CF);
            VF = ((A>>7) == (((~DATA + CF)>>7)&1)) && (((temp>>7)&1) != A>>7);
            CF = temp>>8;       // borrow
            A = temp;
            ZF_NF(A);
            break;
            
        case 0xF9:
            /* printf("SBC Absolute, Y\n"); */
            ABSOLUTE_Y(cpu);
            cycles = 4;
            PAGE_CROSSED;
            // Taking two's complement of data
            temp = A + ((uint8_t)(~DATA) + CF);
            VF = ((A>>7) == (((~DATA + CF)>>7)&1)) && (((temp>>7)&1) != A>>7);
            CF = temp>>8;       // borrow
            A = temp;
            ZF_NF(A);
            break;

        case 0xE1:
            /* printf("SBC Indexed Indirect\n"); */
            INDEXED_INDIRECT(cpu);
            // Taking two's complement of data
            temp = A + ((uint8_t)(~DATA) + CF);
            VF = ((A>>7) == (((~DATA + CF)>>7)&1)) && (((temp>>7)&1) != A>>7);
            CF = temp>>8;       // borrow
            A = temp;
            ZF_NF(A);
            cycles = 6;
            break;

        case 0xF1:
            /* printf("SBC Indirect Indexed\n"); */
            INDIRECT_INDEXED(cpu);
            cycles = 5;
            PAGE_CROSSED;
            // Taking two's complement of data
            temp = A + ((uint8_t)(~DATA) + CF);
            VF = ((A>>7) == (((~DATA + CF)>>7)&1)) && (((temp>>7)&1) != A>>7);
            CF = temp>>8;       // borrow
            A = temp;
            ZF_NF(A);
            break;

        // AND
        case 0x29:
            /* printf("AND Immediate\n"); */
            IMMEDIATE(cpu);
            A &= DATA;
            ZF_NF(A);
            break;

        case 0x25:
            /* printf("AND Zero Page\n"); */
            ZERO_PAGE(cpu);
            A &= DATA;
            ZF_NF(A);
            cycles = 3;
            break;

        case 0x35:
            /* printf("AND Zero Page X\n"); */
            ZERO_PAGE_X(cpu);
            A &= DATA;
            ZF_NF(A);
            cycles = 4;
            break;

        case 0x2D:
            /* printf("AND Absolute\n"); */
            ABSOLUTE(cpu);
            A &= DATA;
            ZF_NF(A);
            cycles = 4;
            break;

        case 0x3D:
            /* printf("AND Absolute X\n"); */
            ABSOLUTE_X(cpu);
            cycles = 4;
            PAGE_CROSSED;
            A &= DATA;
            ZF_NF(A);
            break;
            
        case 0x39:
            /* printf("AND Absolute Y\n"); */
            ABSOLUTE_Y(cpu);
            cycles = 4;
            PAGE_CROSSED;
            A &= DATA;
            ZF_NF(A);
            break;
            
        case 0x21:
            /* printf("AND Indexed Indirect\n"); */
            INDEXED_INDIRECT(cpu);
            A &= DATA;
            ZF_NF(A);
            cycles = 6;
            break;

        case 0x31:
            /* printf("AND Indirect Indexed\n"); */
            INDIRECT_INDEXED(cpu);
            cycles = 5;
            PAGE_CROSSED;
            A &= DATA;
            ZF_NF(A);
            break;

        // ORA
        case 0x09:
            /* printf("ORA Immediate\n"); */
            IMMEDIATE(cpu);
            A |= DATA;
            ZF_NF(A);
            break;

        case 0x05:
            /* printf("ORA Zero Page\n"); */
            ZERO_PAGE(cpu);
            A |= DATA;
            ZF_NF(A);
            cycles = 3;
            break;

        case 0x15:
            /* printf("ORA Zero Page X\n"); */
            ZERO_PAGE_X(cpu);
            A |= DATA;
            ZF_NF(A);
            cycles = 4;
            break;

        case 0x0D:
            /* printf("ORA Absolute\n"); */
            ABSOLUTE(cpu);
            A |= DATA;
            ZF_NF(A);
            cycles = 4;
            break;

        case 0x1D:
            /* printf("ORA Absolute X\n"); */
            ABSOLUTE_X(cpu);
            cycles = 4;
            PAGE_CROSSED;
            A |= DATA;
            ZF_NF(A);
            break;

        case 0x19:
            /* printf("ORA Absolute Y\n"); */
            ABSOLUTE_Y(cpu);
            cycles = 4;
            PAGE_CROSSED;
            A |= DATA;
            ZF_NF(A);
            break;

        case 0x01:
            /* printf("ORA Indexed Indirect\n"); */
            INDEXED_INDIRECT(cpu);
            A |= DATA;
            ZF_NF(A);
            cycles = 6;
            break;

        case 0x11:
            /* printf("ORA Indirect Indexed\n"); */
            INDIRECT_INDEXED(cpu);
            cycles = 5;
            PAGE_CROSSED;
            A |= DATA;
            ZF_NF(A);
            break;

        // SRE Unoffical
        
        case 0x43:
            INDEXED_INDIRECT(cpu);
            CF = DATA & 1;
            DATA = DATA >> 1;
            cpu->cpu_write(cpu);
            A ^= DATA;
            ZF_NF(A);
            cycles = 8;
            break;

        case 0x47:
            ZERO_PAGE(cpu);
            CF = DATA & 1;
            DATA = DATA >> 1;
            cpu->mem[bal] = DATA;
            A ^= DATA;
            ZF_NF(A);
            cycles = 5;
            break;
        case 0x57:
            ZERO_PAGE_X(cpu);
            CF = DATA & 1;
            DATA = DATA >> 1;
            cpu->mem[bal] = DATA;
            A ^= DATA;
            ZF_NF(A);
            cycles = 6;
            break;
        case 0x4F:
            ABSOLUTE(cpu);
            CF = DATA & 1;
            DATA = DATA >> 1;
            cpu->cpu_write(cpu);
            A ^= DATA;
            ZF_NF(A);
            cycles = 6;
            break;
        case 0x5F:
            ABSOLUTE_X(cpu);
            CF = DATA & 1;
            DATA = DATA >> 1;
            cpu->cpu_write(cpu);
            A ^= DATA;
            ZF_NF(A);
            cycles = 7;
            break;
        case 0x5B:
            ABSOLUTE_Y(cpu);
            CF = DATA & 1;
            DATA = DATA >> 1;
            cpu->cpu_write(cpu);
            A ^= DATA;
            ZF_NF(A);
            cycles = 7;
            break;
         case 0x53:
            INDIRECT_INDEXED(cpu);
            CF = DATA & 1;
            DATA = DATA >> 1;
            cpu->cpu_write(cpu);
            A ^= DATA;
            ZF_NF(A);
            cycles = 8;
            break;           

        // EOR
        case 0x49:
            /* printf("EOR Immediate\n"); */
            IMMEDIATE(cpu);
            A ^= DATA;
            ZF_NF(A);
            break;

        case 0x45:
            /* printf("EOR Zero Page\n"); */
            ZERO_PAGE(cpu);
            A ^= DATA;
            ZF_NF(A);
            cycles = 3;
            break;

        case 0x55:
            /* printf("EOR Zero Page X\n"); */
            ZERO_PAGE_X(cpu);
            A ^= DATA;
            ZF_NF(A);
            cycles = 4;
            break;
            
        case 0x4D:
            /* printf("EOR Absolute\n"); */
            ABSOLUTE(cpu);
            A ^= DATA;
            ZF_NF(A);
            cycles = 4;
            break;

        case 0x5D:
            /* printf("EOR Absolute X\n"); */
            ABSOLUTE_X(cpu);
            cycles = 4;
            PAGE_CROSSED;
            A ^= DATA;
            ZF_NF(A);
            break;

        case 0x59:
            /* printf("EOR Absolute Y\n"); */
            ABSOLUTE_Y(cpu);
            cycles = 4;
            PAGE_CROSSED;
            A ^= DATA;
            ZF_NF(A);
            break;

        case 0x41:
            /* printf("EOR Indexed Indirect\n"); */
            INDEXED_INDIRECT(cpu);
            A ^= DATA;
            ZF_NF(A);
            cycles = 6;
            break;

        case 0x51:
            /* printf("EOR Indirect Indexed\n"); */
            INDIRECT_INDEXED(cpu);
            cycles = 5;
            PAGE_CROSSED;
            A ^= DATA;
            ZF_NF(A);
            break;

        case 0x18:
            /* printf("CLC Implied\n"); */
            CF = 0;
            break;

        case 0x38:
            /* printf("SEC Implied\n"); */
            CF = 1;
            break;

        case 0x58:
            /* printf("CLI Implied\n"); */
            IDF = 0;
            break;

        case 0x78:
            /* printf("SEI Implied\n"); */
            IDF = 1;
            break;

        case 0xB8:
            /* printf("CLV Implied\n"); */
            VF = 0;
            break;

        case 0xD8:
            /* printf("CLD Implied\n"); */
            DF = 0;
            break;

        case 0xF8:
            /* printf("SED Implied\n"); */
            DF = 1;
            break;

        case 0x4C:
            /* printf("JMP Absolute\n"); */
            FETCH_ADL(cpu);     // T1
            FETCH_ADH(cpu);     // T2
            cpu->pc = (adh<<8)|adl;
            cycles = 3;
            break;

        case 0x6C:
            FETCH_ADL(cpu);     // T1
            FETCH_ADH(cpu);     // T2
            
            cpu->address_bus = (adh << 8) | adl;
            cpu->cpu_read(cpu);
            cpu->pc = cpu->data_bus;   // T3 PCL
            cpu->address_bus = (adh << 8) | ((adl + 1) & 0xFF);
            cpu->cpu_read(cpu);
            cpu->pc = (cpu->data_bus<<8)|cpu->pc;
            cycles = 5;
            break;

        // BPL
        case 0x10:
            /* printf("BPL Relative\n"); */
            RELATIVE(cpu, (NF == 0));
            break;

        // BMI
        case 0x30:
            /* printf("BMI Relative\n"); */
            RELATIVE(cpu, NF);
            break;

        case 0x50:
            /* printf("BVC Relative\n"); */
            RELATIVE(cpu, (VF == 0));
            break;

        case 0x70:
            /* printf("BVS Relative\n"); */
            RELATIVE(cpu, VF);
            break;

        case 0x90:
            /* printf("BCC Relative\n"); */
            RELATIVE(cpu, (CF == 0));
            break;

        case 0xB0:
            /* printf("BCS Relative\n"); */
            RELATIVE(cpu, CF);
            break;

        case 0xD0:
            /* printf("BNE Relative %0X\n", cpu->pc); */
            /* printf("ZF - %0X\n", ZF); */
            RELATIVE(cpu, (ZF == 0));
            break;

        case 0xF0:
            /* printf("BEQ Relative\n"); */
            RELATIVE(cpu, ZF);
            break;

            //CMP
        case 0xC9:
            /* printf("CMP Immediate   A - %0X\n", A); */
            IMMEDIATE(cpu);
            temp = A - DATA;
            temp &= 255;        // clearing carry
            CF = DATA <= A;
            ZF_NF(temp);
            break;

        case 0xC5:
            /* printf("CMP Zero Page\n"); */
            ZERO_PAGE(cpu);
            temp = A - DATA;
            temp &= 255;        // clearing carry
            CF = DATA <= A;
            ZF_NF(temp);
            cycles = 3;
            break;

        case 0xD5:
            /* printf("CMP Zero Page X\n"); */
            ZERO_PAGE_X(cpu);
            /* printf("A - %0X\n", DATA); */
            temp = A - DATA;
            temp &= 255;        // clearing carry
            CF = DATA <= A;
            ZF_NF(temp);
            cycles = 4;
            break;

        case 0xCD:
            /* printf("CMP Absolute\n"); */
            ABSOLUTE(cpu);
            temp = A - DATA;
            temp &= 255;        // clearing carry
            CF = DATA <= A;
            ZF_NF(temp);
            cycles = 4;
            break;

        case 0xDD:
            /* printf("CMP Absolute X A - %0X\n", A); */
            ABSOLUTE_X(cpu);
            /* printf("DATA - %0X\n", DATA); */
            cycles = 4;
            PAGE_CROSSED;
            temp = A - DATA;
            temp &= 255;        // clearing carry
            CF = DATA <= A;
            ZF_NF(temp);
            /* printf("ZF - %0X\n", PC); */
            break;

        case 0xD9:
            /* printf("CMP Absolute Y\n"); */
            ABSOLUTE_Y(cpu);
            cycles = 4;
            PAGE_CROSSED;
            temp = A - DATA;
            temp &= 255;        // clearing carry
            CF = DATA <= A;
            ZF_NF(temp);
            break;

        case 0xC1:
            /* printf("CMP Indexed Indirect\n"); */
            INDEXED_INDIRECT(cpu);
            temp = A - DATA;
            temp &= 255;        // clearing carry
            CF = DATA <= A;
            ZF_NF(temp);
            cycles = 6;
            break;

        case 0xD1:
            /* printf("CMP Indirect Indexed\n"); */
            INDIRECT_INDEXED(cpu);
            cycles = 5;
            PAGE_CROSSED;
            temp = A - DATA;
            temp &= 255;        // clearing carry
            CF = DATA <= A;
            ZF_NF(temp);
            break;

        // DCP Unoffical Opcode
        
        case 0xC7:
            // Zero Page DCP
            ZERO_PAGE(cpu);
            DATA -= 1;
            cpu->mem[bal] = DATA;

            temp = A - DATA;
            temp &= 255;        // clearing carry
            CF = DATA <= A;
            ZF_NF(temp);

            cycles = 5;
            break;

        case 0xD7:
            // Zero Page X DCP
            ZERO_PAGE_X(cpu);
            DATA -= 1;
            cpu->mem[(uint8_t)bal] = DATA;

            temp = A - DATA;
            temp &= 255;        // clearing carry
            CF = DATA <= A;
            ZF_NF(temp);
            cycles = 6;
            break;

        case 0xCF:
            // Absolute DCP
            ABSOLUTE(cpu);
            DATA -= 1;
            cpu->cpu_write(cpu);

            temp = A - DATA;
            temp &= 255;        // clearing carry
            CF = DATA <= A;
            ZF_NF(temp);
            cycles = 6;
            break;
            
        case 0xDF:
            // Absolute X DCP
            ABSOLUTE_X(cpu);
            DATA -= 1;
            cpu->cpu_write(cpu);

            temp = A - DATA;
            temp &= 255;        // clearing carry
            CF = DATA <= A;
            ZF_NF(temp);
            cycles = 7;
            break;

        case 0xDB:
            // Absolute Y DCP
            ABSOLUTE_Y(cpu);
            DATA -= 1;
            cpu->cpu_write(cpu);

            temp = A - DATA;
            temp &= 255;        // clearing carry
            CF = DATA <= A;
            ZF_NF(temp);
            cycles = 7;
            break;

        case 0xC3:
            // INDEXED_INDIRECT DCP
            INDEXED_INDIRECT(cpu);
            DATA -= 1;
            cpu->cpu_write(cpu);

            temp = A - DATA;
            temp &= 255;        // clearing carry
            CF = DATA <= A;
            ZF_NF(temp);

            cycles = 8;
            break;

        case 0xD3:
            // INDIRECT_INDEXED DCP
            INDIRECT_INDEXED(cpu);
            DATA -= 1;
            cpu->cpu_write(cpu);

            temp = A - DATA;
            temp &= 255;        // clearing carry
            CF = DATA <= A;
            ZF_NF(temp);
            cycles = 8;
            break;

        // BIT
        case 0x24:
            /* printf("BIT Zero Page\n"); */
            ZERO_PAGE(cpu);
            temp = DATA & A;
            ZF = (temp == 0);
            NF = DATA>>7;
            VF = (DATA>>6)&1;
            cycles = 3;
            break;

        case 0x2C:
            /* printf("BIT Absolute\n"); */
            ABSOLUTE(cpu);
            temp = DATA & A;
            ZF = (temp == 0);
            NF = DATA>>7;
            VF = (DATA>>6)&1;
            cycles = 4;
            break;
            
        // LDX
        case 0xA2:
            /* printf("LDX Immediate\n"); */
            IMMEDIATE(cpu);
            X = DATA;
            /* printf("X - %0X\n", DATA); */
            ZF_NF(X);
            break;

        case 0xA6:
            /* printf("LDX Zero Page\n"); */
            ZERO_PAGE(cpu);
            X = DATA;
            ZF_NF(X);
            cycles = 3;
            break;

        case 0xB6:
            /* printf("LDX Zero Page Y\n"); */
            ZERO_PAGE_Y(cpu);
            X = DATA;
            ZF_NF(X);
            cycles = 4;
            break;

        case 0xAE:
            /* printf("LDX Absolute\n"); */
            ABSOLUTE(cpu);
            X = DATA;
            ZF_NF(X);
            cycles = 4;
            break;
            
        case 0xBE:
            /* printf("LDX Absolute Y\n"); */
            ABSOLUTE_Y(cpu);
            cycles = 4;
            PAGE_CROSSED;
            X = DATA;
            ZF_NF(X);
            break;

        // LDY
        case 0xA0:
            /* printf("LDY Immediate\n"); */
            IMMEDIATE(cpu);
            Y = DATA;
            ZF_NF(Y);
            break;

        case 0xA4:
            /* printf("LDY Zero Page\n"); */
            ZERO_PAGE(cpu);
            Y = DATA;
            ZF_NF(Y);
            cycles = 3;
            break;

        case 0xB4:
            /* printf("LDY Zero Page X\n"); */
            ZERO_PAGE_X(cpu);
            Y = DATA;
            ZF_NF(Y);
            cycles = 4;
            break;
            
        case 0xAC:
            /* printf("LDY Absolute\n"); */
            ABSOLUTE(cpu);
            Y = DATA;
            ZF_NF(Y);
            cycles = 4;
            break;

        case 0xBC:
            /* printf("LDY Absolute X\n"); */
            ABSOLUTE_X(cpu);
            cycles = 4;
            PAGE_CROSSED;
            Y = DATA;
            ZF_NF(Y);
            break;

        // STX
        case 0x86:
            /* printf("STX Zero Page\n"); */
            W_ZERO_PAGE(cpu, X);
            cpu->rw = 1;        // write operation is over
            break;

        case 0x96:
            /* printf("STX Zero Page Y\n"); */
            W_ZERO_PAGE_Y(cpu, X);
            cpu->rw = 1;
            break;

        case 0x8E:
            /* printf("STX Absolute\n"); */
            W_ABSOLUTE(cpu, X);
            cpu->rw = 1;
            break;

        // STY
        case 0x84:
            /* printf("STY Zero Page\n"); */
            W_ZERO_PAGE(cpu, Y);
            cpu->rw = 1;
            break;

        case 0x94:
            /* printf("STY Zero Page X - %0X\n", X); */
            W_ZERO_PAGE_X(cpu, Y);
            cpu->rw = 1;
            break;

        case 0x8C:
            /* printf("STY Absolute\n"); */
            W_ABSOLUTE(cpu, Y);
            cpu->rw = 1;
            break;

        case 0xE8:
            /* printf("INX Implied\n"); */
            X += 1;
            ZF_NF(X);
            break;

        case 0xC8:
            /* printf("INY Implied\n"); */
            Y += 1;
            ZF_NF(Y);
            break;

        case 0xCA:
            /* printf("DEX Implied\n"); */
            X -= 1;
            ZF_NF(X);
            break;

        case 0x88:
            /* printf("DEY Implied\n"); */
            Y -= 1;
            ZF_NF(Y);
            break;

        // CPX
        case 0xE0:
            /* printf("CPX Immediate\n"); */
            IMMEDIATE(cpu);
            temp = X - DATA;
            temp &= 255;
            CF = X >= DATA;
            ZF_NF(temp);
            break;

        case 0xE4:
            /* printf("CPX Zero Page\n"); */
            ZERO_PAGE(cpu);
            temp = X - DATA;
            temp &= 255;
            CF = X >= DATA;
            ZF_NF(temp);
            cycles = 3;
            break;

        case 0xEC:
            /* printf("CPX Absolute\n"); */
            ABSOLUTE(cpu);
            temp = X - DATA;
            temp &= 255;
            CF = X >= DATA;
            ZF_NF(temp);
            cycles = 4;
            break;

        // CPY
        case 0xC0:
            /* printf("CPY Immediate\n"); */
            IMMEDIATE(cpu);
            temp = Y - DATA;
            temp &= 255;
            CF = Y >= DATA;
            ZF_NF(temp);
            break;

        case 0xC4:
            /* printf("CPY Zero Page\n"); */
            ZERO_PAGE(cpu);
            temp = Y - DATA;
            temp &= 255;
            CF = Y >= DATA;
            ZF_NF(temp);
            cycles = 3;
            break;

        case 0xCC:
            /* printf("CPY Absolute\n"); */
            ABSOLUTE(cpu);
            temp = Y - DATA;
            temp &= 255;
            CF = Y >= DATA;
            ZF_NF(temp);
            cycles = 4;
            break;

        case 0xAA:
            /* printf("TAX Implied\n"); */
            X = A;
            ZF_NF(X);
            break;

        case 0x8A:
            /* printf("TXA Implied\n"); */
            A = X;
            ZF_NF(A);
            break;

        case 0xA8:
            /* printf("TAY Implied\n"); */
            Y = A;
            ZF_NF(Y);
            break;

        case 0x98:
            /* printf("TYA Implied\n"); */
            A = Y;
            ZF_NF(Y);
            break;

        // STACK OPERATIONS
        
        case 0x20:
            /* printf("JSR Absolute\n"); */
            FETCH_ADL(cpu);     // T1
            // Store ADL        // T2
            // PCH
            PUSH_STACK(cpu, (cpu->pc >> 8));        // T3
            cpu->rw = 1;
            // PCL
            PUSH_STACK(cpu, (cpu->pc & 255));       // T4
            cpu->rw = 1;
            FETCH_ADH(cpu);                         // T5
            cpu->pc = (adh<<8)|adl;
            cycles = 6;
            break;

        case 0x60:
            /* printf("RTS Implied\n"); */
            // Decode RTS       // T1
            PULL_STACK(cpu);    // T2, T3
            adl = cpu->data_bus;
            PULL_STACK(cpu);    // T4
            adh = cpu->data_bus;

            cpu->pc = (adh<<8)|adl; // T5
            cpu->pc += 1;       // T5
            cycles = 6;
            break;

        case 0x48:
            /* printf("PHA Implied\n"); */
            // Interpret PHA. Hold P-Counter    \\ T1
            PUSH_STACK(cpu, A);     // T2
            cpu->rw = 1;
            cycles = 3;
            break;

        case 0x68:
            /* printf("PLA Implied\n"); */
            // Interpret instruction, Hold P-Counter        // T1
            PULL_STACK(cpu);        // T2
            A = cpu->data_bus;      // T3
            /* printf("S - %0X, A - %0X\n", A, SP); */
            ZF_NF(A);
            cycles = 4;
            break;

        case 0x9A:
            /* printf("TXS Implied\n"); */
            SP = X;
            break;

        case 0xBA:
            /* printf("TSX Implied\n"); */
            X = SP;
            ZF_NF(X);
            break;
            
        case 0x08:
            /* printf("PHP Implied\n"); */
            PUSH_STACK(cpu, PS.value | 0x10);
            cpu->rw = 1;
            cycles = 3;
            break;

        case 0x28:
            /* printf("PLP Implied\n"); */
            temp = BF;
            PULL_STACK(cpu);
            PS.value = cpu->data_bus;
            BF = temp;
            NUF = 1;        // not used flag bit
            cycles = 4;
            break;

        case 0x40:
            /* printf("RTI Implied\n"); */
            // Decode RTI   // T1
            temp = BF;
            PULL_STACK(cpu);        // T2 and T3 // FLAGS
            PS.value = cpu->data_bus;
            NUF = 1;    // not used flag always on
            BF = temp;
            // Fetch PCL
            PULL_STACK(cpu);    // T4
            cpu->pc = cpu->data_bus;
            // Fetch PCH
            PULL_STACK(cpu);        // T5
            cpu->pc = (cpu->data_bus << 8) | cpu->pc;
            cycles = 6;
            break;

        case 0x00:
            /* printf("BRK Implied\n"); */
            cpu->pc += 1;
            // Now PC is already BRK + 2 . There is no holding of pc
            PUSH_STACK(cpu, (cpu->pc >> 8));    // T1, T2 PCH
            PUSH_STACK(cpu, (cpu->pc & 255));   // T3 PCL
            BF = 1;
            PUSH_STACK(cpu, PS.value);    // T4
            cpu->rw = 1;
            FETCH_ADDR(cpu, IRQ_VECTOR_LOW);  // T5
            adl = cpu->data_bus;
            FETCH_ADDR(cpu, IRQ_VECTOR_HIGH); // T6
            IDF = 1;
            adh = cpu->data_bus;
            cpu->pc = (adh << 8)|adl;
            cycles = 7;
            break;

        // Shift & Rotate
        // The accumulator mode only takes 2 cycles
        
        case 0x4A:
            /* printf("LSR Accumulator\n"); */
            SHIFT_RIGHT(A);
            /* CF = A & 1; */
            /* A = A >> 1; */
            /* NF = 0; */
            /* ZF = A == 0; */
            break;

        case 0x46:
            /* printf("LSR Zero Page\n"); */
            ZERO_PAGE(cpu);     // T1 and T2
            // T3
            SHIFT_RIGHT(DATA);
            cpu->rw = 0;
            // T3
            cpu->mem[bal] = DATA;       // T4
            cpu->rw = 1;
            cycles = 5;
            break;

        case 0x56:
            /* printf("LSR Zero Page X\n"); */
            ZERO_PAGE_X(cpu);   // T1 and T2
            // T3
            SHIFT_RIGHT(DATA);
            cpu->rw = 0;
            // T3
            cpu->mem[bal] = DATA;       // T4
            cpu->rw = 1;
            cycles = 6;
            break;

        case 0x4E:
            /* printf("LSR Absolute\n"); */
            ABSOLUTE(cpu);      // T1 and T2 and T3
            // T4
            SHIFT_RIGHT(DATA);
            cpu->rw = 0;
            // T4
            /* cpu->mem[cpu->address_bus] = DATA;  // T5 */
            cpu->cpu_write(cpu);
            cpu->rw = 1;
            cycles = 6;
            break;

        case 0x5E:
            /* printf("LSR Absolute X\n"); */
            ABSOLUTE_X(cpu);
            // T5
            SHIFT_RIGHT(DATA);
            cpu->rw = 0;
            // T5
            /* cpu->mem[cpu->address_bus] = DATA;  // T6 */
            cpu->cpu_write(cpu);
            cpu->rw = 1;
            cycles = 7;
            break;

        case 0x0A:
            /* printf("ASL Accumulator\n"); */
            /* CF = A>>7; */
            /* A = A << 1; */
            /* ZF_NF(A); */
            SHIFT_LEFT(A);
            break;

        case 0x06:
            /* printf("ASL Zero Page\n"); */
            ZERO_PAGE(cpu);     // T1 and T2
            // T3
            SHIFT_LEFT(DATA);
            /* printf("DATA - %0X\n", DATA); */
            cpu->rw = 0;
            // T3
            cpu->mem[bal] = DATA;   // T4
            cpu->rw = 1;
            cycles = 5;
            break;
            
        case 0x16:
            /* printf("ASL Zero Page X\n"); */
            ZERO_PAGE_X(cpu);     // T1 and T2
            // T3
            SHIFT_LEFT(DATA);
            cpu->rw = 0;
            // T3
            cpu->mem[bal] = DATA;   // T4
            cpu->rw = 1;
            cycles = 6;
            break;

        case 0x0E:
            /* printf("ASL Absolute\n"); */
            ABSOLUTE(cpu);      // T1 and T2 and T3
            // T4
            SHIFT_LEFT(DATA);
            cpu->rw = 0;
            // T4
            /* cpu->mem[cpu->address_bus] = DATA;   // T5 */
            cpu->cpu_write(cpu);
            cpu->rw = 1;
            cycles = 6;
            break;

        // SLO Unoffical
        case 0x03:
            // Indexed Indirect SLO
            INDEXED_INDIRECT(cpu);
            CF = DATA >> 7;
            DATA = DATA << 1;
            cpu->cpu_write(cpu);
            A |= DATA;
            ZF_NF(A);
            cycles = 8;
            break;

        case 0x07:
            // Zero Page SLO
            ZERO_PAGE(cpu);
            CF = DATA >> 7;
            DATA = DATA << 1;
            cpu->mem[bal] = DATA;
            A |= DATA;
            ZF_NF(A);
            cycles = 5;
            break;

        case 0x17:
            // Zero Page X SLO
            ZERO_PAGE_X(cpu);
            CF = DATA >> 7;
            DATA = DATA << 1;
            cpu->mem[bal] = DATA;
            A |= DATA;
            ZF_NF(A);
            cycles = 6;
            break;
        case 0x0F:
            // ABSOLUTE SLO
            ABSOLUTE(cpu);
            CF = DATA >> 7;
            DATA = DATA << 1;
            cpu->cpu_write(cpu);
            A |= DATA;
            ZF_NF(A);
            cycles = 6;
            break;
        case 0x1F:
            // ABSOLUTE X SLO
            ABSOLUTE_X(cpu);
            CF = DATA >> 7;
            DATA = DATA << 1;
            cpu->cpu_write(cpu);
            A |= DATA;
            ZF_NF(A);
            cycles = 7;
            break;
        case 0x1B:
            // ABSOLUTE Y SLO
            ABSOLUTE_Y(cpu);
            CF = DATA >> 7;
            DATA = DATA << 1;
            cpu->cpu_write(cpu);
            A |= DATA;
            ZF_NF(A);
            cycles = 7;
            break;
        case 0x13:
            // INDIRECT_INDEXED SLO
            INDIRECT_INDEXED(cpu);
            CF = DATA >> 7;
            DATA = DATA << 1;
            cpu->cpu_write(cpu);
            A |= DATA;
            ZF_NF(A);
            cycles = 8;
            break;

        case 0x1E:
            /* printf("ASL Absolute X\n"); */
            /* FETCH_ADL(cpu);     // T1 */
            /* FETCH_ADH(cpu);     // T2 */
            /* cpu->address_bus = (adh<<8)|(adl + X);      // T3 */
            /* DATA = cpu->mem[cpu->address_bus];          // T4 */
            ABSOLUTE_X(cpu);
            // T5
            SHIFT_LEFT(DATA);
            cpu->rw = 0;
            // T5
            /* cpu->mem[cpu->address_bus] = DATA;      // T6 */
            cpu->cpu_write(cpu);
            cpu->rw = 1;
            cycles = 7;
            break;

        // RLA unoffical opcode
        case 0x23:
            INDEXED_INDIRECT(cpu);
            ROTAT_LEFT_FL(DATA);
            cpu->cpu_write(cpu);
            A &= DATA;
            ZF_NF(A);
            cycles = 8;
            break;
        case 0x27:
            ZERO_PAGE(cpu);
            ROTAT_LEFT_FL(DATA);
            cpu->mem[bal] = DATA;
            A &= DATA;
            ZF_NF(A);
            cycles = 5;
            break;

        case 0x37:
            ZERO_PAGE_X(cpu);
            ROTAT_LEFT_FL(DATA);
            cpu->mem[bal] = DATA;
            A &= DATA;
            ZF_NF(A);
            cycles = 6;
            break;
        case 0x2F:
            ABSOLUTE(cpu);
            ROTAT_LEFT_FL(DATA);
            cpu->cpu_write(cpu);
            A &= DATA;
            ZF_NF(A);
            cycles = 6;
            break;
        case 0x3F:
            ABSOLUTE_X(cpu);
            ROTAT_LEFT_FL(DATA);
            cpu->cpu_write(cpu);
            A &= DATA;
            ZF_NF(A);
            cycles = 7;
            break;
        case 0x3B:
            ABSOLUTE_Y(cpu);
            ROTAT_LEFT_FL(DATA);
            cpu->cpu_write(cpu);
            A &= DATA;
            ZF_NF(A);
            cycles = 7;
            break;
        case 0x33:
            INDIRECT_INDEXED(cpu);
            ROTAT_LEFT_FL(DATA);
            cpu->cpu_write(cpu);
            A &= DATA;
            ZF_NF(A);
            cycles = 8;
            break;

        case 0x2A:
            /* printf("ROL Accumulator\n"); */
            /* temp = CF; */
            /* CF = A>>7; */
            /* A = (A << 1)|temp; */
            /* ZF_NF(A); */
            ROTATE_LEFT(A);
            break;

        case 0x26:
            /* printf("ROL Zero Page\n"); */
            ZERO_PAGE(cpu);     // T1 and T2
            // T3
            ROTATE_LEFT(DATA);
            cpu->rw = 0;
            // T3
            cpu->mem[bal] = DATA;   // T4
            cpu->rw = 1;
            cycles = 5;
            break;

        case 0x36:
            /* printf("ROL Zero Page X\n"); */
            ZERO_PAGE_X(cpu);       // T1 and T2
            // T3
            ROTATE_LEFT(DATA);
            cpu->rw = 0;
            // T3
            cpu->mem[bal] = DATA;   // T4
            cpu->rw = 1;
            cycles = 6;
            break;

        case 0x2E:
            /* printf("ROL Absolute\n"); */
            ABSOLUTE(cpu);      // T1 and T2 and T3
            // T4
            ROTATE_LEFT(DATA);
            cpu->rw = 0;
            // T4
            /* cpu->mem[cpu->address_bus] = DATA;   // T5 */
            cpu->cpu_write(cpu);
            cpu->rw = 1;
            cycles = 6;
            break;

        case 0x3E:
            /* printf("ROL Absolute X\n"); */
            ABSOLUTE_X(cpu);
            // T5
            ROTATE_LEFT(DATA);
            cpu->rw = 0;
            // T5
            cpu->cpu_write(cpu);
            /* cpu->mem[cpu->address_bus] = DATA;      // T6 */
            cpu->rw = 1;
            cycles = 7;
            break;

        // RRA Unoffical
        case 0x63:
            INDEXED_INDIRECT(cpu);
            ROTATE_RIGHT_FL(DATA);
            cpu->cpu_write(cpu);
            temp = A + DATA + CF;
            CF = temp>>8;   // carry bit
            VF = ((A>>7) == (DATA>>7)) && (((temp>>7)&1) != A>>7);
            A = temp;
            ZF_NF(A);
            cycles = 8;
            break;

        case 0x67:
            ZERO_PAGE(cpu);
            ROTATE_RIGHT_FL(DATA);
            cpu->mem[bal] = DATA;
            temp = A + DATA + CF;
            CF = temp>>8;   // carry bit
            VF = ((A>>7) == (DATA>>7)) && (((temp>>7)&1) != A>>7);
            A = temp;
            ZF_NF(A);
            cycles = 5;
            break;
        case 0x77:
            ZERO_PAGE_X(cpu);
            ROTATE_RIGHT_FL(DATA);
            cpu->mem[bal] = DATA;
            temp = A + DATA + CF;
            CF = temp>>8;   // carry bit
            VF = ((A>>7) == (DATA>>7)) && (((temp>>7)&1) != A>>7);
            A = temp;
            ZF_NF(A);
            cycles = 6;
            break;
        case 0x6F:
            ABSOLUTE(cpu);
            ROTATE_RIGHT_FL(DATA);
            cpu->cpu_write(cpu);
            temp = A + DATA + CF;
            CF = temp>>8;   // carry bit
            VF = ((A>>7) == (DATA>>7)) && (((temp>>7)&1) != A>>7);
            A = temp;
            ZF_NF(A);
            cycles = 6;
            break;
        case 0x7F:
            ABSOLUTE_X(cpu);
            ROTATE_RIGHT_FL(DATA);
            cpu->cpu_write(cpu);
            temp = A + DATA + CF;
            CF = temp>>8;   // carry bit
            VF = ((A>>7) == (DATA>>7)) && (((temp>>7)&1) != A>>7);
            A = temp;
            ZF_NF(A);
            cycles = 7;
            break;
        case 0x7B:
            ABSOLUTE_Y(cpu);
            ROTATE_RIGHT_FL(DATA);
            cpu->cpu_write(cpu);
            temp = A + DATA + CF;
            CF = temp>>8;   // carry bit
            VF = ((A>>7) == (DATA>>7)) && (((temp>>7)&1) != A>>7);
            A = temp;
            ZF_NF(A);
            cycles = 7;
            break;
        case 0x73:
            INDIRECT_INDEXED(cpu);
            ROTATE_RIGHT_FL(DATA);
            cpu->cpu_write(cpu);
            temp = A + DATA + CF;
            CF = temp>>8;   // carry bit
            VF = ((A>>7) == (DATA>>7)) && (((temp>>7)&1) != A>>7);
            A = temp;
            ZF_NF(A);
            cycles = 8;
            break;

        case 0x6A:
            /* printf("ROR Accumulator\n"); */
            /* temp = CF; */
            /* NF = CF; */
            /* CF = A & 1; */
            /* A = (A >> 1) | (temp << 7); */
            /* ZF = A == 0; */
            ROTATE_RIGHT(A);
            break;

        case 0x66:
            /* printf("ROR Zero Page\n"); */
            ZERO_PAGE(cpu);     // T1 and T2
            ROTATE_RIGHT(DATA); // T3
            cpu->rw = 0;
            cpu->mem[bal] = DATA;   // T4
            cpu->rw = 1;
            cycles = 5;
            break;

        case 0x76:
            /* printf("ROR Zero Page X\n"); */
            ZERO_PAGE_X(cpu);       // T1 and T2
            ROTATE_RIGHT(DATA);     // T3
            cpu->rw = 0;
            /* cpu->cpu_write(cpu); */
            cpu->mem[bal] = DATA;  // T4
            cpu->rw = 1;
            cycles = 6;
            break;
            
        case 0x6E:
            /* printf("ROR Absolute\n"); */
            ABSOLUTE(cpu);      // T1 and T2 and T3
            ROTATE_RIGHT(DATA); // T4
            cpu->rw = 0;
            cpu->cpu_write(cpu);
            /* cpu->mem[cpu->address_bus] = DATA; */
            cpu->rw = 1;
            cycles = 6;
            break;

        case 0x7E:
            /* printf("ROR Absolute X\n"); */
            ABSOLUTE_X(cpu);
            ROTATE_RIGHT(DATA);     // T5
            cpu->rw = 0;
            cpu->cpu_write(cpu);
            /* cpu->mem[cpu->address_bus] = DATA;      // T6 */
            cpu->rw = 1;
            cycles = 7;
            break;

        // INC DEC
        case 0xE6:
            /* printf("INC Zero Page\n"); */
            ZERO_PAGE(cpu);
            DATA += 1;
            ZF_NF(DATA);
            cpu->rw = 0;
            cpu->mem[bal] = DATA;
            cpu->rw = 1;
            cycles = 5;
            break;

        case 0xF6:
            /* printf("INC Zero Page X\n"); */
            ZERO_PAGE_X(cpu);
            DATA += 1;
            ZF_NF(DATA);
            cpu->rw = 0;
            cpu->mem[bal] = DATA;
            cpu->rw = 1;
            cycles = 6;
            break;

        case 0xEE:
            /* printf("INC Absolute\n"); */
            ABSOLUTE(cpu);
            DATA += 1;
            ZF_NF(DATA);
            cpu->rw = 0;
            /* cpu->mem[cpu->address_bus] = DATA; */
            cpu->cpu_write(cpu);
            cpu->rw = 1;
            cycles = 6;
            break;

        case 0xFE:
            /* printf("INC Absolute X\n"); */
            ABSOLUTE_X(cpu);
            DATA += 1;
            ZF_NF(DATA);
            cpu->rw = 0;
            cpu->cpu_write(cpu);
            /* cpu->mem[cpu->address_bus] = DATA; */
            cpu->rw = 1;
            cycles = 7;
            break;

        case 0xC6:
            /* printf("DEC Zero Page\n"); */
            ZERO_PAGE(cpu);
            DATA -= 1;
            ZF_NF(DATA);
            cpu->rw = 0;
            cpu->mem[bal] = DATA;
            cpu->rw = 1;
            cycles = 5;
            break;

        case 0xD6:
            /* printf("DEC Zero Page X\n"); */
            ZERO_PAGE_X(cpu);
            DATA -= 1;
            ZF_NF(DATA);
            cpu->rw = 0;
            cpu->mem[bal] = DATA;
            cpu->rw = 1;
            cycles = 6;
            break;

        case 0xCE:
            /* printf("DEC Absolute\n"); */
            ABSOLUTE(cpu);
            DATA -= 1;
            ZF_NF(DATA);
            cpu->rw = 0;
            /* cpu->mem[cpu->address_bus] = DATA; */
            cpu->cpu_write(cpu);
            cpu->rw = 1;
            cycles = 6;
            break;

        case 0xDE:
            /* printf("DEC Absolute X\n"); */
            ABSOLUTE_X(cpu);
            DATA -= 1;
            ZF_NF(DATA);
            cpu->rw = 0;
            cpu->cpu_write(cpu);
            /* cpu->mem[cpu->address_bus] = DATA; */
            cpu->rw = 1;
            cycles = 7;
            break;

        case 0xEA:
            /* printf("NOP Implied\n"); */
            break;

        // Unoffical OP Codes
        case 0x1A:
            // Implied NOP
            break;

        case 0x3A:
            // Implied NOP
            break;
        case 0x5A:
            // Implied NOP
            break;

        case 0x7A:
            // Implied NOP
            break;

        case 0xDA:
            // Implied NOP
            break;

        case 0xFA:
            // Implied NOP
            break;

        case 0x04:
            // Zero Page NOP
            cpu->pc += 1;
            cycles = 3;
            break;

        case 0x14:
            // Zero Page X NOP
            cpu->pc += 1;
            cycles = 4;
            break;

        case 0x34:
            // Zero Page X NOP
            cpu->pc += 1;
            cycles = 4;
            break;

        case 0x44:
            // Zero Page NOP
            cpu->pc += 1;
            cycles = 3;
            break;

        case 0x54:
            // Zero Page X NOP
            cpu->pc += 1;
            cycles = 4;
            break;

        case 0x64:
            // Zero Page NOP
            cpu->pc += 1;
            cycles = 3;
            break;

        case 0x74:
            // Zero Page X NOP
            cpu->pc += 1;
            cycles = 4;
            break;

        case 0x80:
            // Immediate NOP
            cpu->pc += 1;
            break;

        case 0x82:
            // Immediate NOP
            cpu->pc += 1;
            break;

        case 0x89:
            // Immediate NOP
            cpu->pc += 1;
            break;

        case 0xC2:
            // Immediate NOP
            cpu->pc += 1;
            break;

        case 0xD4:
            // Zero Page X NOP
            cpu->pc += 1;
            cycles = 4;
            break;

        case 0xE2:
            // Immediate NOP
            cpu->pc += 1;
            break;

        case 0xF4:
            // Zero Page X NOP
            cpu->pc += 1;
            cycles = 4;
            break;

        case 0x0C:
            // Absolute NOP
            cpu->pc += 2;
            cycles = 4;
            break;

        case 0x1C:
            // Absolute X NOP
            ABSOLUTE_X(cpu);
            cycles = 4;
            PAGE_CROSSED;
            break;

        case 0x3C:
            // Absolute X NOP
            ABSOLUTE_X(cpu);
            cycles = 4;
            PAGE_CROSSED;
            break;

        case 0x5C:
            // Absolute X NOP
            ABSOLUTE_X(cpu);
            cycles = 4;
            PAGE_CROSSED;
            break;

        case 0x7C:
            // Absolute X NOP
            ABSOLUTE_X(cpu);
            cycles = 4;
            PAGE_CROSSED;
            break;

        case 0xDC:
            // Absolute X NOP
            ABSOLUTE_X(cpu);
            cycles = 4;
            PAGE_CROSSED;
            break;

        case 0xFC:
            // Absolute X NOP
            ABSOLUTE_X(cpu);
            cycles = 4;
            PAGE_CROSSED;
            break;

        default:
            
            printf("%0X Wrong opcode %0x\n", cpu->pc, cpu->ir);
            exit(1);
    }
    cpu->total_cycles += cycles;
    return cycles;
}


#undef RESET_VECTOR_LOW
#undef RESET_VECTOR_HIGH

#undef IRQ_VECTOR_LOW
#undef IRQ_VECTOR_HIGH

#undef NMI_VECTOR_LOW
#undef NMI_VECTOR_HIGH
