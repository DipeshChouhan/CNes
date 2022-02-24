#include "../mos_6502/cpu_impl.h"
#include "ppu.h"
#include "nes.h"
#include "mappers.h"
#include <stdio.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_events.h>
#include <SDL2/SDL_pixels.h>
#include <SDL2/SDL_render.h>
#include <SDL2/SDL_timer.h>
#include <SDL2/SDL_video.h>
#include "controllers/controllers.h"

#define HORIZONTAL_V 0b1111101111100000
#define VERTICAL_V   0b1000010000011111

// copied from nesdev.org
#define INC_HORIZONTAL_V(_v)                    \
    if ((_v & 0x001F) == 31){                   \
        _v &= ~0x001F;                          \
        _v ^= 0x0400;                           \
    } else {                                    \
        _v += 1;                                \
    }                                           \

// copied from nesdev.org
#define INC_VERTICAL_V(_v)                      \
    if ((_v & 0x7000) != 0x7000){               \
        _v += 0x1000;                           \
    } else {                                    \
        _v &= ~0x7000;                          \
        temp = (_v & 0x03E0) >> 5;              \
        if (temp == 29){                        \
            temp = 0;                           \
            _v ^= 0x0800;                       \
        } else if (temp == 31) {                \
            temp = 0;                           \
        } else {                                \
            temp += 1;                          \
        }                                       \
        _v = (_v & ~0x03E0) | (temp << 5);      \
    }                                           \

#define OAMDMA_CYCLE(_cpu)                                                  \
    if (oamdma_count < 512) {                                               \
        if ((oamdma_count & 1) == 0) {                                      \
            _cpu->address_bus = cpu_oamdma_addr++;                          \
            _cpu->cpu_read(_cpu);                                           \
            ppu->oam[oamdma_addr & 0xFF] = _cpu->data_bus;                  \
            ++oamdma_addr;                                                  \
        }                                                                   \
        cycles = 3;                                                         \
        ++oamdma_count;                                                     \
    } else {                                                                \
        cycles = 3;                                                         \
        if (cpu->total_cycles & 1) {                                        \
            cycles *= 3;                                                    \
        }                                                                   \
        ppu->oam_dma = 0;                                                   \
        oamdma_count = 0;                                                   \
    }                                                                       \


#define CPU_CYCLE(_cpu)                     \
    if (cycles == 0) {                      \
        if (ppu->oam_dma) {                 \
            OAMDMA_CYCLE(_cpu);             \
        } else {                            \
            cycles = cpu_cycle(_cpu) * 3;   \
        }                                   \
    }                                       \


#define AT_X ((VRAM_X(ppu->v) / 2) % 2)
#define AT_Y ((VRAM_Y(ppu->v) / 2) % 2)

#define LOAD_BG_REGISTERS(_bg_latch)                    \
    bg_low = bg_low | (_bg_latch & 0xFF);               \
    bg_high = bg_high | ((_bg_latch >> 8) & 0xFF);      \
    at_low = at_low | ((at_latch & 1) ? 0xFF : 0x00);   \
    at_high = at_high | ((at_latch & 2) ? 0xFF : 0x00); \

#define UPDATE_BG_REGISTERS()       \
        bg_low <<= 1;               \
        bg_high <<= 1;              \
        at_low <<= 1;               \
        at_high <<= 1;              \


#define BACKGROUND_TILES()                                                          \
        switch(ppu_cycle % 8) {                                                     \
            case 1:                                                                 \
                addr_latch = ppu->get_mirrored_addr(ppu->v & 0xFFF);                \
                LOAD_BG_REGISTERS(bg_latch);                                        \
                break;                                                              \
            case 2:                                                                 \
                nt = ppu->nametables[addr_latch];                                   \
                break;                                                              \
            case 3:                                                                 \
                addr_latch = ppu->get_mirrored_addr((0x23C0 | (ppu->v & 0x0C00) | ((ppu->v >> 4) & 0x38) | ((ppu->v >> 2) & 0x07)) & 0xFFF);  \
                break;                                                              \
            case 4:                                                                 \
                at_latch = ppu->nametables[addr_latch];                             \
                at_latch = (at_latch >> (((AT_Y << 1) | AT_X) << 1)) & 3;           \
                break;                                                              \
            case 5:                                                                 \
                addr_latch = (PPUCTRL_BG(ppu->ppu_ctrl) << 12) | (nt << 4) |        \
                    VRAM_FINE_Y(ppu->v);                                            \
                break;                                                              \
            case 6:                                                                 \
                bg_latch = mapper->chr_read(addr_latch);                            \
                break;                                                              \
            case 7:                                                                 \
                addr_latch |= 8;                                                    \
                break;                                                              \
            case 0:                                                                 \
                bg_latch |= (mapper->chr_read(addr_latch) << 8);                    \
                if (RENDERING_ENABLED(ppu->ppu_mask)) {                             \
                    INC_HORIZONTAL_V(ppu->v);                                       \
                    if (ppu_cycle == 256) {                                         \
                        INC_VERTICAL_V(ppu->v);                                     \
                    }                                                               \
                }                                                                   \
                break;                                                              \
        }                                                                           \

#define TWO_BACKGROUND_TILES()                                                      \
        switch(ppu_cycle % 8) {                                                     \
            case 1:                                                                 \
                addr_latch = ppu->get_mirrored_addr(ppu->v & 0xFFF);                \
                LOAD_BG_REGISTERS(bg_latch);                                        \
                break;                                                              \
            case 2:                                                                 \
                nt = ppu->nametables[addr_latch];                                   \
                break;                                                              \
            case 3:                                                                 \
                addr_latch = ppu->get_mirrored_addr((0x23C0 | (ppu->v & 0x0C00) | ((ppu->v >> 4) & 0x38) | ((ppu->v >> 2) & 0x07)) & 0xFFF);  \
                break;                                                              \
            case 4:                                                                 \
                at_latch = ppu->nametables[addr_latch];                             \
                at_latch = (at_latch >> (((AT_Y << 1) | AT_X) << 1)) & 3;           \
                break;                                                              \
            case 5:                                                                 \
                addr_latch = (PPUCTRL_BG(ppu->ppu_ctrl) << 12) | (nt << 4) |        \
                    VRAM_FINE_Y(ppu->v);                                            \
                break;                                                              \
            case 6:                                                                 \
                bg_latch = mapper->chr_read(addr_latch);                            \
                break;                                                              \
            case 7:                                                                 \
                addr_latch |= 8;                                                    \
                break;                                                              \
            case 0:                                                                 \
                bg_latch |= (mapper->chr_read(addr_latch) << 8);                    \
                if (RENDERING_ENABLED(ppu->ppu_mask)) {                             \
                    INC_HORIZONTAL_V(ppu->v);                                       \
                }                                                                   \
                break;                                                              \
        }                                                                           \


void ppu_render(struct Cpu *cpu) {

    unsigned int static_pallete[0x40] = {

        0xFF808080, 0xFF003DA6, 0xFF0012B0, 0xFF440096, 0xFFA1005E,
        0xFFC70028, 0xFFBA0600, 0xFF8C1700, 0xFF5C2F00, 0xFF104500,
        0xFF054A00, 0xFF00472E, 0xFF004166, 0xFF000000, 0xFF050505,
        0xFF050505, 0xFFC7C7C7, 0xFF0077FF, 0xFF2155FF, 0xFF8237FA,
        0xFFEB2FB5, 0xFFFF2950, 0xFFFF2200, 0xFFD63200, 0xFFC46200,
        0xFF358000, 0xFF058F00, 0xFF008A55, 0xFF0099CC, 0xFF212121,
        0xFF090909, 0xFF090909, 0xFFFFFFFF, 0xFF0FD7FF, 0xFF69A2FF,
        0xFFD480FF, 0xFFFF45F3, 0xFFFF618B, 0xFFFF8833, 0xFFFF9C12,
        0xFFFABC20, 0xFF9FE30E, 0xFF2BF035, 0xFF0CF0A4, 0xFF05FBFF,
        0xFF5E5E5E, 0xFF0D0D0D, 0xFF0D0D0D, 0xFFFFFFFF, 0xFFA6FCFF,
        0xFFB3ECFF, 0xFFDAABEB, 0xFFFFA8F9, 0xFFFFABB3, 0xFFFFD2B0,
        0xFFFFEFA6, 0xFFFFF79C, 0xFFD7E895, 0xFFA6EDAF, 0xFFA2F2DA,
        0xFF99FFFC, 0xFFDDDDDD, 0xFF111111, 0xFF111111,
    };

    int oamdma_count= 0;

    SDL_Window *window;
    SDL_Renderer *renderer;
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Couldn't initialize SDL: %s", SDL_GetError());
        exit(3);
    }
    window = SDL_CreateWindow("NesBox", SDL_WINDOWPOS_CENTERED,
            SDL_WINDOWPOS_CENTERED, 768, 240*3, SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE);
    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_TARGETTEXTURE | 
            SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    SDL_Texture *texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888, 
            SDL_TEXTUREACCESS_STREAMING, 256, 240);
    unsigned int colors[4] = {0x00000000, 0xFF4f4f4f, 0xFF828282, 0xFFFFFFFF};
    unsigned int pixels[256 * 240];
    int pitch = 256 * sizeof(unsigned int);
    int pixel_count = 0;

    int temp = 0;
    int scanline = 0;
    int ppu_cycle = 0;
    int cycles = 21;
    int frames = 0;

    int bg_low = 0;
    int bg_high = 0;

    int bg_latch = 0;

    int addr_latch = 0;

    int nt = 0;
    int at_low = 0;
    int at_high = 0;
    int at_latch = 0;
    int pixel_mux = 0;
    char bg_palette = 0;
    char bg_pixel = 0;

    // sprites
    int oam_count = 0;
    int oam_count2 = 0;
    unsigned oam_data = 0;
    int sprite_in_range = 0;
    char sprite_zero_next = 1;
    char sprite_zero_current = 0;

    int sprites_to_render = 0;
    int sprites_x[8];
    unsigned char sprites_attrs[8];
    unsigned char sprites_low[8];
    unsigned char sprites_high[8];
    char sprite_pixel = 0;
    char sprite_palette = 0;
    char sprite_priority = 0;
    char sprite_flip = 0;
    char check_sprite_zero_hit = 0;

    if (cycles == 0) {
        cycles = cpu_cycle(cpu) * 3;
    }

VISIBLE_SCANLINES:
    CPU_CYCLE(cpu);
    --cycles;
    if (ppu_cycle == 0) {
        ++ppu_cycle;
        ppu->total_sprites = 0; // secondary oam clear
        sprite_in_range = 0;
        oam_count2 = 0;
        goto VISIBLE_SCANLINES;
    }

    if (ppu_cycle < 257) {
        BACKGROUND_TILES();


        pixel_mux = PPUCTRL_SPRITE_SIZE(ppu->ppu_ctrl) ? 15 : 7; 

        if (ppu_cycle > 64 && oam_count < 256) {

            if (ppu_cycle & 1) {
                // cycle is odd
                if (ppu_cycle == 65) {
                    oam_count = ppu->oam_addr;   // evaluation starts at this oam address(sprite zero) 
                }
                oam_data = ppu->oam[oam_count];
            } else {
                // cycle is even
                // check of range
                if (sprite_in_range == 0 && scanline >= oam_data && (scanline - oam_data) <= pixel_mux) {
                    if (sprite_zero_next == 1) {
                        sprite_zero_next = 2;
                    }
                    oam_data = scanline - oam_data;
                    sprite_in_range = 4;
                    if (ppu->total_sprites == 8) {
                        PPUSTATUS_SET_OVERFLOW(ppu->ppu_status);
                        sprite_in_range = 0;
                        oam_count = 256;
                    } else {
                        ++ppu->total_sprites;
                    }
                }

                sprite_zero_next &= 2;
                if (sprite_in_range > 0) {
                    ppu->oam2[oam_count2++] = oam_data;
                    ++oam_count;
                    --sprite_in_range;
                } else {
                    oam_count += 4;
                }
            }

        }

        temp = 0;
        check_sprite_zero_hit = 0;
        while (temp < sprites_to_render) {
            sprite_flip = sprites_attrs[temp] & 0b01000000;
            if (sprites_x[temp] == 1) {
                if (sprite_flip) {

                    if (sprite_pixel == 0) {

                        sprite_pixel = ((sprites_high[temp] & 1) << 1) | (sprites_low[temp] & 1);
                        sprite_palette = sprites_attrs[temp] & 3;
                        sprite_priority = sprites_attrs[temp] & 0b00100000;
                        if (temp == 0 && sprite_pixel > 0) {
                            check_sprite_zero_hit = 1;
                        }
                    }
                        sprites_low[temp] >>= 1;
                        sprites_high[temp] >>= 1;

                } else {

                    if (sprite_pixel == 0) {

                        sprite_pixel = ((sprites_high[temp] >> 7) << 1) | (sprites_low[temp] >> 7);
                        sprite_palette = sprites_attrs[temp] & 3;
                        sprite_priority = sprites_attrs[temp] & 0b00100000;
                        if (temp == 0 && sprite_pixel > 0) {
                            check_sprite_zero_hit = 1;
                        }
                    }

                        sprites_low[temp] <<= 1;
                        sprites_high[temp] <<= 1;
                }
            } else {
                if (PPUMASK_SPRITE_SHOW(ppu->ppu_mask)) {
                    --sprites_x[temp];
                }
            }
            ++temp;
        }


        pixel_mux = 0x8000 >> ppu->fine_x;
        bg_pixel = (((bg_high & pixel_mux) > 0) << 1) | ((bg_low & pixel_mux) > 0);
        bg_palette = (((at_high & pixel_mux) > 0) << 1) | ((at_low & pixel_mux) > 0);

        
        if (PPUMASK_BG_SHOW(ppu->ppu_mask) == 0) {
            bg_pixel = 0;
        } else if (ppu_cycle < 9 && PPUMASK_BG_LEFTMOST(ppu->ppu_mask) == 0){
            bg_pixel = 0;
        }
        if (PPUMASK_SPRITE_SHOW(ppu->ppu_mask) == 0) {
            sprite_pixel = 0;
        } else if (ppu_cycle < 9 && PPUMASK_SPRITE_LEFTMOST(ppu->ppu_mask) == 0) {
            sprite_pixel = 0;
        }
        

        if (RENDERING_ENABLED(ppu->ppu_mask)) {
            if (bg_pixel) {
                if (sprite_pixel) {
                    // SPRITE ZERO HIT
                    if (PPUSTATUS_ZEROHIT(ppu->ppu_status) == 0 && sprite_zero_current == 2 && ppu_cycle < 256 && check_sprite_zero_hit) {
                        PPUSTATUS_SET_ZEROHIT(ppu->ppu_status);
                    }
                    if (sprite_priority == 0) {
                        pixels[pixel_count] = static_pallete[ppu->palette[0x10 + (sprite_palette * 4) + sprite_pixel]];
                    } else {
                        pixels[pixel_count] = static_pallete[ppu->palette[(bg_palette * 4) + bg_pixel]];
                    }
                    sprite_pixel = 0;
                } else {
                    // bg 
                    pixels[pixel_count] = static_pallete[ppu->palette[(bg_palette * 4) + bg_pixel]];
                }
            } else if (sprite_pixel) {
                    pixels[pixel_count] = static_pallete[ppu->palette[0x10 + (sprite_palette * 4) + sprite_pixel]];
                    sprite_pixel = 0;

            } else {
                // back drop color
                pixels[pixel_count] = static_pallete[ppu->palette[0]];
            }
        } else {
                if (ppu->v >= 0x3F00) {
                    pixels[pixel_count] = static_pallete[ppu->palette[ppu->v & 0x1F]];
                } else {
                    pixels[pixel_count] = static_pallete[ppu->palette[0]];
                }
                sprite_pixel = 0;
        }

        ++pixel_count;
        ++ppu_cycle;
        UPDATE_BG_REGISTERS();
        goto VISIBLE_SCANLINES;

    } 
    if(RENDERING_ENABLED(ppu->ppu_mask)) {
        ppu->v = (ppu->v & HORIZONTAL_V) | (ppu->t & VERTICAL_V);
    }
    ++ppu_cycle;
    oam_count = 0;
    sprites_to_render = 0;
    ppu->oam_addr = 0;

SPRITE_FETCH:
    CPU_CYCLE(cpu);
    --cycles;

    if (ppu_cycle < 321) {
        ppu->oam_addr = 0;
        // 8x8 sprites only
        if (sprites_to_render < ppu->total_sprites) {
            switch(ppu_cycle % 8) {
                case 5:
                    // low sprite tile byte
                    sprites_x[sprites_to_render] = ppu->oam2[oam_count + 3] + 1;
                    sprites_attrs[sprites_to_render] = ppu->oam2[oam_count + 2];
                    sprite_flip = sprites_attrs[sprites_to_render] & 0x80;
                    if (PPUCTRL_SPRITE_SIZE(ppu->ppu_ctrl)) {
                        // 8x16 sprite
                        addr_latch = ((ppu->oam2[oam_count + 1] & 1) << 12) | 
                            ((ppu->oam2[oam_count + 1] & 0b11111110) << 4);

                        if (sprite_flip) {
                            addr_latch = addr_latch | (((ppu->oam2[oam_count] ^ 8) & 8) << 1) | (7 - ppu->oam2[oam_count] & 7);
                        } else {
                            addr_latch = addr_latch | ((ppu->oam2[oam_count] & 8) << 1) | (ppu->oam2[oam_count] & 7);
                        }
                    } else {
                        addr_latch = (PPUCTRL_SPRITE(ppu->ppu_ctrl) << 12) | 
                            (ppu->oam2[oam_count + 1] << 4) |
                            (sprite_flip ? (7 - ppu->oam2[oam_count]) : ppu->oam2[oam_count]);
                    }
                    break;

                case 6:
                    // low sprite tile byte
                    sprites_low[sprites_to_render] = mapper->chr_read(addr_latch);
                    break;

                case 7:
                    // high sprite tile byte
                    addr_latch |= 8;
                    break;

                case 0:
                    // high sprite tile byte
                    sprites_high[sprites_to_render] = mapper->chr_read(addr_latch);
                    ++sprites_to_render;
                    oam_count += 4;
                    break;
            }
        }
        ++ppu_cycle;
        goto SPRITE_FETCH;
    }

    sprite_zero_current = sprite_zero_next;
    sprite_zero_next = 1;
    // 321
    ++ppu_cycle;
    addr_latch = ppu->get_mirrored_addr(ppu->v & 0xFFF);
    UPDATE_BG_REGISTERS();

TWO_TILES:
    CPU_CYCLE(cpu);
    --cycles;

    if (ppu_cycle < 337) {
        TWO_BACKGROUND_TILES();
        ++ppu_cycle;
        UPDATE_BG_REGISTERS();
        goto TWO_TILES;
    }

    if (ppu_cycle == 340) {
        // next scanline
        ++scanline;
        ppu_cycle = 0;
        if (scanline == 240) {
            goto INVISIBLE_SCANLINES;
        }
        goto VISIBLE_SCANLINES;
    }
    ++ppu_cycle;
    goto TWO_TILES;

INVISIBLE_SCANLINES:
    CPU_CYCLE(cpu);
    --cycles;
    if (scanline == 241 && ppu_cycle == 1) {
        // set vblank
        PPUSTATUS_SET_VBLANK(ppu->ppu_status);
        if (PPUCTRL_NMI(ppu->ppu_ctrl)) {
            cpu->nmi = 1;
        }
    } else {
        if (ppu_cycle == 340) {
            // inc scanline
            ++scanline;
            ppu_cycle = 0;
            if (scanline == 261) {
                goto PRE_RENDER_SCANLINE;
            }
            goto INVISIBLE_SCANLINES;
        }
    }
    ++ppu_cycle;
    goto INVISIBLE_SCANLINES;

PRE_RENDER_SCANLINE:
    CPU_CYCLE(cpu);
    --cycles;

    if (ppu_cycle == 0) {
        ++ppu_cycle;
        goto PRE_RENDER_SCANLINE;
    }

    if (ppu_cycle == 1) {
        // clear vblank sprite and overflow
        ppu->ppu_status &= 0b00011111;
        sprite_zero_current = 0;
        sprites_to_render = 0;
        sprite_zero_next = 1;
        ++ppu_cycle;
        goto PRE_RENDER_SCANLINE;
    }

    if (ppu_cycle < 257) {
        switch(ppu_cycle % 8) {
            case 0:
                // inc horizontal(v)
                if (RENDERING_ENABLED(ppu->ppu_mask)) {
                    INC_HORIZONTAL_V(ppu->v);
                    if (ppu_cycle == 256) {
                        INC_VERTICAL_V(ppu->v);
                    }
                }
                break;
        }
        ++ppu_cycle;
        goto PRE_RENDER_SCANLINE;
    }


    // horizontal (v) = horizontal(t)
    if(RENDERING_ENABLED(ppu->ppu_mask)) {
        ppu->v = (ppu->v & HORIZONTAL_V) | (ppu->t & VERTICAL_V);
    }
    ++ppu_cycle;

    temp = 0b000000000100000;
    ppu->oam_addr = 0;

PRE_SPRITE_FETCH:
    CPU_CYCLE(cpu);
    --cycles;

    
    if (ppu_cycle < 321) {
        ppu->oam_addr = 0;
        if (ppu_cycle >= 285 && ppu_cycle <= 304) {
            // copy vertical(v) = vertical(t)
            if (ppu_cycle & 1) {
                // odd cycle
                oam_count2 = ppu->t & temp;

            } else {
                // even cycle
                if (temp != 1024 && RENDERING_ENABLED(ppu->ppu_mask)) {
                    ppu->v = (ppu->v & (~temp)) | oam_count2;
                }
                temp <<= 1;
            }
        }
        ++ppu_cycle;
        goto PRE_SPRITE_FETCH;
    }

    // 321
    ++ppu_cycle;
    addr_latch = ppu->get_mirrored_addr(ppu->v & 0xFFF);
    UPDATE_BG_REGISTERS();
    

PRE_TWO_TILES:
    CPU_CYCLE(cpu);
    --cycles;

    if (ppu_cycle < 337) {
        TWO_BACKGROUND_TILES();
        UPDATE_BG_REGISTERS();
        ++ppu_cycle;
        goto PRE_TWO_TILES;
    }

    if (ppu_cycle == 339) {
        if (RENDERING_ENABLED(ppu->ppu_mask) && (frames & 1)) {
            scanline = 0;
            ppu_cycle = 0;
            ++frames;
            pixel_count = 0;
            SDL_UpdateTexture(texture, NULL, pixels, pitch);
            SDL_RenderCopy(renderer, texture, NULL, NULL);
            SDL_RenderPresent(renderer);
            joypad1_events();
            // render
            goto VISIBLE_SCANLINES;
        }
    }

    if (ppu_cycle == 340) {
        scanline = 0;
        ppu_cycle = 0;
        ++frames;
        pixel_count = 0;
        SDL_UpdateTexture(texture, NULL, pixels, pitch);
        SDL_RenderCopy(renderer, texture, NULL, NULL);
        SDL_RenderPresent(renderer);
        joypad1_events();
        // render
        goto VISIBLE_SCANLINES;
    }
    ++ppu_cycle;
    goto PRE_TWO_TILES;

}

