#ifndef _PPU_H
#define _PPU_H

#include "../mos_6502/cpu_impl.h"

#define NAMETABLES_SIZE 0x800       // 2 name_tables

        /* Address range	Size	Description */
        /* $0000-$0FFF	$1000	Pattern table 0 */
        /* $1000-$1FFF	$1000	Pattern table 1 */
        /* $2000-$23FF	$0400	Nametable 0 */
        /* $2400-$27FF	$0400	Nametable 1 */
        /* $2800-$2BFF	$0400	Nametable 2 */
        /* $2C00-$2FFF	$0400	Nametable 3 */
        /* $3000-$3EFF	$0F00	Mirrors of $2000-$2EFF */
        /* $3F00-$3F1F	$0020	Palette RAM indexes */
        /* $3F20-$3FFF	$00E0	Mirrors of $3F00-$3F1F */

// Masks for ppu registers
// For PPUCTRL
#define PPUCTRL_NTA(_ppu_ctrl)          (_ppu_ctrl & 3)
#define PPUCTRL_VINC(_ppu_ctrl)         ((_ppu_ctrl >> 2) & 1)
#define PPUCTRL_SPRITE(_ppu_ctrl)       ((_ppu_ctrl >> 3) & 1)
#define PPUCTRL_BG(_ppu_ctrl)           ((_ppu_ctrl >> 4) & 1)
#define PPUCTRL_SPRITE_SIZE(_ppu_ctrl)  ((_ppu_ctrl >> 5) & 1)
#define PPUCTRL_BACKDROP(_ppu_ctrl)     ((_ppu_ctrl >> 6) & 1)
#define PPUCTRL_NMI(_ppu_ctrl)          (_ppu_ctrl & 0b10000000)

#define PPUMASK_GREYSCALE(_ppu_mask)        (_ppu_mask & 1)
#define PPUMASK_BG_LEFTMOST(_ppu_mask)      (_ppu_mask & 0b00000010)
#define PPUMASK_SPRITE_LEFTMOST(_ppu_mask)  (_ppu_mask & 0b00000100)
#define PPUMASK_BG_SHOW(_ppu_mask)          (_ppu_mask & 0b00001000)
#define PPUMASK_SPRITE_SHOW(_ppu_mask)      (_ppu_mask & 0b00010000)
#define PPUMASK_RED(_ppu_mask)              (_ppu_mask & 0b00100000)
#define PPUMASK_GREEN(_ppu_mask)            (_ppu_mask & 0b01000000)
#define PPUMASK_BLUE(_ppu_mask)             (_ppu_mask & 0b10000000)

#define RENDERING_ENABLED(_ppu_mask)        (_ppu_mask & 0b00011000)

#define PPUSTATUS_OVERFLOW(_ppu_status) (_ppu_status & 0b00100000)
#define PPUSTATUS_ZEROHIT(_ppu_status)  (_ppu_status & 0b01000000)
#define PPUSTATUS_VBLANK(_ppu_status)   (_ppu_status & 0b10000000)

#define PPUSTATUS_SET_OVERFLOW(_ppu_status)             \
    _ppu_status = (_ppu_status | 0b00100000);           \

#define PPUSTATUS_CLEAR_OVERFLOW(_ppu_status)           \
    _ppu_status = _ppu_status & 0b11011111;             \

#define PPUSTATUS_SET_ZEROHIT(_ppu_status)              \
    _ppu_status = _ppu_status | 0b01000000;              \

#define PPUSTATUS_CLEAR_ZEROHIT(_ppu_status)            \
    _ppu_status = _ppu_status & 0b10111111;             \

#define PPUSTATUS_SET_VBLANK(_ppu_status)               \
    _ppu_status = _ppu_status | 0b10000000;              \

#define PPUSTATUS_CLEAR_VBLANK(_ppu_status)             \
    _ppu_status = _ppu_status & 0b01111111;             \


#define VRAM_FINE_Y(_vram_addr)          ((_vram_addr >> 12) & 7)
#define VRAM_NT(_vram_addr)              ((_vram_addr >> 10) & 3)
#define VRAM_Y(_vram_addr)               ((_vram_addr >> 5) & 31)
#define VRAM_X(_vram_addr)               (_vram_addr & 31)

#define PPUCTRL_WRITE(_vram_addr, _ppu_ctrl)                        \
    _vram_addr = (_vram_addr & 0b111001111111111) | ((_ppu_ctrl & 3) << 10);         \

#define PPUSCROLL_FIRST_WRITE(_vram_addr, _data, _x)                    \
    _vram_addr = (_vram_addr & 0b111111111100000) | (_data >> 3);       \
    _x = _data & 7;                                                     \

#define PPUSCROLL_SECOND_WRITE(_vram_addr, _data)                           \
    _vram_addr = (_vram_addr & 0b000110000011111) | ((_data & 248) << 2);   \
    _vram_addr = (_vram_addr | ((_data & 7) << 12));                          \

#define PPUADDR_FIRST_WRITE(_vram_addr, _data)                              \
    _vram_addr = (_vram_addr & 0b000000011111111) | ((_data & 63) << 8);    \

#define PPUADDR_SECOND_WRITE(_vram_addr, _data)                             \
    _vram_addr = (_vram_addr & 0b111111100000000) | _data;                  \




typedef struct {
    unsigned char nametables[NAMETABLES_SIZE];
    unsigned char palette[0x20];
    unsigned char data_latch;
    unsigned char oam[256];
    unsigned char oam2[32];
    unsigned char total_sprites;
    int v;  // Current VRAM address (15 bits)
    int t;  // Temporary VRAM address (15 bits);can also be
            //thought of as the address of the top left onscreen tile.
    int fine_x;  // Fine X scroll (3 bits)
    int write_toggle; // First or second write toggle (1 bit); 0 or 1
    unsigned char read_buffer;
    unsigned char vblank;
    unsigned char ppu_ctrl;
    unsigned char ppu_mask;
    unsigned char ppu_status;
    unsigned char oam_addr;
    char oam_dma;
    int (*get_mirrored_addr)(int);
} Ppu;


void ppu_render(struct Cpu *cpu);

#endif
