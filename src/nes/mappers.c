#include <stdio.h>
#include "mappers.h"
#include "nes.h"
#include "ppu.h"
// TODO: Reading PPUSTATUS will clear vblank flag of PPUSTATUS register [IMP]
//TODO: Check PPUDATA Read and Write for correctness
    /* Address range	Size	Device */
    /* $0000-$07FF	$0800	2KB internal RAM */
    /* $0800-$0FFF	$0800	Mirrors of $0000-$07FF */
    /* $1000-$17FF	$0800 */
    /* $1800-$1FFF	$0800 */
    /* $2000-$2007	$0008	NES PPU registers */
    /* $2008-$3FFF	$1FF8	Mirrors of $2000-2007 (repeats every 8 bytes) */
    /* $4000-$4017	$0018	NES APU and I/O registers */
    /* $4018-$401F	$0008	APU and I/O functionality that is normally disabled. See CPU Test Mode. */
    /* $4020-$FFFF	$BFE0	Cartridge space: PRG ROM, PRG RAM, and mapper registers (See Note) */
#define PPUCTRL 0x2000
#define PPUMASK 0x2001
#define PPUSTATUS 0x2002
#define OAMADDR 0x2003
#define OAMDATA 0x2004
#define PPUSCROLL 0x2005
#define PPUADDR 0x2006
#define PPUDATA 0x2007
#define OAMDMA 0x4014

#define JOYPAD1 0x4016
#define JOYPAD2 0x4017


unsigned char dynamic_latch = 0;

unsigned char joypad1_read_count = 0;

void common_read(struct Cpu *cpu) {
    if (cpu->address_bus < 0x2000) {
        // cpu ram
        cpu->data_bus = cpu->mem[cpu->address_bus & 0x7FF];
        return;
    }

    if (cpu->address_bus < 0x4000) {
        // ppu registers
        switch (cpu->address_bus & 0x2007) {
            case PPUSTATUS:
                // PPUSTATUS    // read
                cpu->data_bus = ppu->ppu_status;
                /* printf("reading ppu_status %d\n", cpu->data_bus); */
                /* exit(0); */
                ppu->write_toggle = 0;
                PPUSTATUS_CLEAR_VBLANK(ppu->ppu_status);
                
                break;
            case OAMDATA:
                // OAMDATA     // read/write
                cpu->data_bus = ppu->oam[cpu->mem[OAMADDR]];
                    // not in vblank or forced blanking

                if (PPUSTATUS_VBLANK(ppu->ppu_status) || !RENDERING_ENABLED(ppu->ppu_mask)) {
                    break;
                }
                ++cpu->mem[OAMADDR]; // increment OAMADDR
                break;
            case PPUDATA:
                // TODO: During rendering it increments coarse X and Y simultaniously
                // PPUDATA      // read/write
                if ((ppu->v & 0x3FFF) < 0x3000) {
                    mapper->vram_read(cpu);
                } else if ((ppu->v & 0x3FFF) > 0x3EFF) {
                    unsigned char temp = (ppu->v & 0x1F);
                    if (temp > 0x0F && (temp % 4) == 0) {
                        temp -= 0x10;
                    }
                    cpu->data_bus = ppu->palette[temp];

                    ppu->read_buffer = ppu->nametables[ppu->get_mirrored_addr(ppu->v - 0x1000)];

                } else {
                    printf("Reading at $%d not accesseble.\n", ppu->v);
                    exit(1);
                }
                ppu->v += (PPUCTRL_VINC(ppu->ppu_ctrl)) * 31 + 1;
                break;
            default:
                cpu->data_bus = dynamic_latch;
                return;
                /* printf("can't read from a write only ppu register %d\n", cpu->address_bus & 0x2007); */
                /* exit(1); */
        }
        dynamic_latch = cpu->data_bus;
        return;
    }

    // joypad 1
    if (cpu->address_bus == JOYPAD1) {
        // read
        if ((cpu->mem[JOYPAD1] & 1) == 1) {
            cpu->data_bus = (joypad1 & 1);
            /* printf("1 read %d\n", cpu->data_bus); */
        } else {
            if (joypad1_read_count == 8) {
                cpu->data_bus = 1;
                /* printf("8 read 1\n"); */
            } else {
                cpu->data_bus = (joypad1 >> joypad1_read_count) & 1;
                /* printf("read %d\n", cpu->data_bus); */
                ++joypad1_read_count;
            }
        }
    }
    

}

void common_write(struct Cpu *cpu) {

    if (cpu->address_bus < 0x2000) {
        // cpu ram
        cpu->mem[cpu->address_bus & 0x7FF] = cpu->data_bus;
        return;
    }

    if (cpu->address_bus < 0x4000) {
        // ppu registers
        switch (cpu->address_bus & 0x2007) {
            case PPUCTRL:
                // PPUCTRL      // write7  bit  0
                /* ---- ---- */
                /* VPHB SINN */
                /* |||| |||| */
                /* |||| ||++- Base nametable address */
                /* |||| ||    (0 = $2000; 1 = $2400; 2 = $2800; 3 = $2C00) */
                /* |||| |+--- VRAM address increment per CPU read/write of PPUDATA */
                /* |||| |     (0: add 1, going across; 1: add 32, going down) */
                /* |||| +---- Sprite pattern table address for 8x8 sprites */
                /* ||||       (0: $0000; 1: $1000; ignored in 8x16 mode) */
                /* |||+------ Background pattern table address (0: $0000; 1: $1000) */
                /* ||+------- Sprite size (0: 8x8 pixels; 1: 8x16 pixels) */
                /* |+-------- PPU master/slave select */
                /* |          (0: read backdrop from EXT pins; 1: output color on EXT pins) */
                /* +--------- Generate an NMI at the start of the */
                /*            vertical blanking interval (0: off; 1: on) */
                
                if (PPUSTATUS_VBLANK(ppu->ppu_status)) {
                    if (PPUCTRL_NMI(ppu->ppu_ctrl) == 0 && (cpu->data_bus & 128)) {
                        cpu->nmi = 1;
                    }
                }
                ppu->ppu_ctrl = cpu->data_bus;
                PPUCTRL_WRITE(ppu->t, ppu->ppu_ctrl);
                break;
            case PPUMASK:
                // PPUMASK      // write
                ppu->ppu_mask = cpu->data_bus;
                break;
            case OAMADDR:
                // OAMADDR      // write
                cpu->mem[OAMADDR] = cpu->data_bus;
                break;
            case OAMDATA:
                // OAMDATA      // read/write
                // TODO: Read below.
                // Writes to OAMDATA during rendering
                // (on the pre-render line and the visible lines 0-239,
                // provided either sprite or background rendering is enabled)
                // do not modify values in OAM,
                // but do perform a glitchy increment of OAMADDR
                // best practice to ignore writes to oamdata during rendering
                if (PPUSTATUS_VBLANK(ppu->ppu_status) || !RENDERING_ENABLED(ppu->ppu_mask)) {
                    ppu->oam[cpu->mem[OAMADDR]] = cpu->data_bus;
                }
                ++cpu->mem[OAMADDR];
                break;
            case PPUSCROLL:
                // PPUSCROLL    // write x2
                if (ppu->write_toggle == 0) {
                    /* t: ....... ...ABCDE <- d: ABCDE... */
                    /* x:              FGH <- d: .....FGH */
                    /* w:                  <- 1 */
                    PPUSCROLL_FIRST_WRITE(ppu->t, cpu->data_bus, ppu->fine_x);
                    ppu->write_toggle = 1;
                } else {
                    /* t: FGH..AB CDE..... <- d: ABCDEFGH */
                    /* w:                  <- 0 */
                    PPUSCROLL_SECOND_WRITE(ppu->t, cpu->data_bus);
                    ppu->write_toggle = 0;
                }
                break;
            case PPUADDR:
                // PPUADDR      // write x2
                if (ppu->write_toggle == 0) {
                    /* t: .CDEFGH ........ <- d: ..CDEFGH */
                    /*        <unused>     <- d: AB...... */
                    /* t: Z...... ........ <- 0 (bit Z is cleared) */
                    /* w:                  <- 1 */
                    PPUADDR_FIRST_WRITE(ppu->t, cpu->data_bus);
                    /* printf("first write %d\n", ppu->t); */
                    ppu->write_toggle  = 1;
                } else {
                    /* t: ....... ABCDEFGH <- d: ABCDEFGH */
                    /* v: <...all bits...> <- t: <...all bits...> */
                    /* w:                  <- 0 */
                    PPUADDR_SECOND_WRITE(ppu->t, cpu->data_bus);
                    /* printf("second write %d\n", ppu->t); */
                    ppu->v = ppu->t & 0b0111111111111111;
                    ppu->write_toggle = 0;
                }
                break;
            case PPUDATA:
                // PPUDATA      // read/write
                // TODO: Mirroring
                if ((ppu->v & 0x3FFF) < 0x3000) {
                    mapper->vram_write(cpu);
                } else if ((ppu->v & 0x3FFF) > 0x3EFF) {

                    unsigned char temp = (ppu->v & 0x1F);
                    if (temp > 0x0F && (temp % 4) == 0) {
                        temp -= 0x10;
                    }
                    ppu->palette[temp] = cpu->data_bus;
                } else {
                    printf("Writing at $%d not accesseble.\n", ppu->v);
                    exit(1);
                }

                ppu->v += (PPUCTRL_VINC(ppu->ppu_ctrl)) * 31 + 1;
                // TODO: during rendering it increment coarse x and y simultaniously {It causing errors [ not done yet]}
                break;
        }
        dynamic_latch = cpu->data_bus;
    }

    // nes apu and io registers
    if (cpu->address_bus == OAMDMA) {
        // OAMDMA       // write
        /* int addr = cpu->data_bus << 8; */
        /* int oam_addr = cpu->mem[OAMADDR]; */
        cpu_oamdma_addr = cpu->data_bus << 8;
        oamdma_addr = cpu->mem[OAMADDR];
        ppu->oam_dma = 1;

        /* for (int i = 0; i < 256; i++) { */
        /*     ppu->oam[oam_addr & 0xFF] = cpu->mem[addr++]; */
        /*     ++oam_addr; */
        /* } */
        return;
    }

    if (cpu->address_bus == JOYPAD1) {
        cpu->mem[JOYPAD1] = cpu->data_bus;
        if (cpu->data_bus & 1){
            joypad1_read_count = 0;
        }
    }
}
