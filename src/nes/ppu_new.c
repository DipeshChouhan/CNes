#include "../mos_6502/cpu_impl.h"
#include "ppu.h"
#include "nes.h"
#include <stdio.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_events.h>
#include <SDL2/SDL_pixels.h>
#include <SDL2/SDL_render.h>
#include <SDL2/SDL_timer.h>
#include <SDL2/SDL_video.h>

#define HORIZONTAL_V 0b1111101111100000
#define VERTICAL_V   0b1000010000011111

#define INC_HORIZONTAL_V(_v)                    \
    if ((_v & 0x001F) == 31){                   \
        _v &= ~0x001F;                          \
        _v ^= 0x0400;                           \
    } else {                                    \
        _v += 1;                                \
    }                                           \

#define INC_VERTICAL_V(_v)                      \
    if ((_v & 0x7000) != 0x7000){               \
        _v += 0x1000;                           \
    } else {                                    \
        _v &= ~0x7000;                          \
        y = (_v & 0x03E0) >> 5;                 \
        if (y == 29){                           \
            y = 0;                              \
            _v ^= 0x0800;                       \
        } else if (y == 31) {                   \
            y = 0;                              \
        } else {                                \
            y += 1;                             \
        }                                       \
        _v = (_v & ~0x03E0) | (y << 5);         \
    }                                           \

#define CPU_CYCLE(_cpu)                 \
    if (cycles == 0) {                  \
        ++number;                       \
        cycles = cpu_cycle(_cpu) * 3;    \
    }                                   \


void print_nametables() {
    // name table 1 
    for (int i = 0; i < 960; i++) {
        if (i % 32 == 0) {
            printf("\n");
        }
        printf(" %d ", ppu->nametables[i]);
    }
    printf("----------------------------------\n");

}
void joypad_events() {
    while(SDL_PollEvent(&event)) {
        if (event.type == SDL_KEYDOWN) {
            switch(event.key.keysym.sym) {
                case SDLK_d:
                    JOYPAD_SET(joypad1, BUTTON_A);
                    break;
                case SDLK_f:
                    JOYPAD_SET(joypad1, BUTTON_B);
                    break;
                case SDLK_RETURN:
                    JOYPAD_SET(joypad1, BUTTON_START);
                    break;
                case SDLK_SPACE:
                    JOYPAD_SET(joypad1, BUTTON_SELECT);
                    break;
                case SDLK_DOWN:
                    JOYPAD_SET(joypad1, BUTTON_DOWN);
                    break;
                case SDLK_UP:
                    JOYPAD_SET(joypad1, BUTTON_UP);
                    break;
                case SDLK_LEFT:
                    JOYPAD_SET(joypad1, BUTTON_LEFT);
                    break;
                case SDLK_RIGHT:
                    JOYPAD_SET(joypad1, BUTTON_RIGHT);
                    break;
            }
        } else if(event.type == SDL_KEYUP) {

            switch(event.key.keysym.sym) {
                case SDLK_d:
                    JOYPAD_CLEAR(joypad1, BUTTON_A);
                    break;
                case SDLK_f:
                    JOYPAD_CLEAR(joypad1, BUTTON_B);
                    break;
                case SDLK_RETURN:
                    JOYPAD_CLEAR(joypad1, BUTTON_START);
                    break;
                case SDLK_SPACE:
                    JOYPAD_CLEAR(joypad1, BUTTON_SELECT);
                    break;
                case SDLK_DOWN:
                    JOYPAD_CLEAR(joypad1, BUTTON_DOWN);
                    break;
                case SDLK_UP:
                    JOYPAD_CLEAR(joypad1, BUTTON_UP);
                    break;
                case SDLK_LEFT:
                    JOYPAD_CLEAR(joypad1, BUTTON_LEFT);
                    break;
                case SDLK_RIGHT:
                    JOYPAD_CLEAR(joypad1, BUTTON_RIGHT);
                    break;
            }
        } else if (event.type == SDL_QUIT) {
            SDL_Quit();
            exit(0);
        }
    }
}
int vertical_mirroring(int addr) {
    if (addr < 0x400) {
        return addr;
    }
    if (addr < 0x800) {
        return (addr & 0x3FF) + 0x400;
    }
    if (addr < 0xC00) {
        return addr & 0x3FF;
    }

    // addr < 0x1000
    return (addr & 0x3FF) + 0x400;
}

int horizontal_mirroring(int addr) {
    if (addr < 0x800) {
        return addr & 0x3FF;
    }

    return (addr & 0x3FF) + 0x400;
}


#define LOAD_SHIFT_REGISTERS(_bg_latch)                 \
    bg_low = bg_low | (_bg_latch & 0xFF);               \
    bg_high = bg_high | ((_bg_latch >> 8) & 0xFF);      \


void ppu_render(struct Cpu *cpu) {

    int number = 0;
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

    int y = 0;
    int scanline = 0;
    int ppu_cycle = 0;
    int cycles = 21;
    int frames = 0;

    int bg_low = 0;
    int bg_high = 0;

    int bg_latch = 0;

    int addr_latch = 0;

    int nt = 0;
    unsigned char bg_pixel = 0;


VISIBLE_SCANLINES:
    CPU_CYCLE(cpu);
    --cycles;
    if (ppu_cycle == 0) {
        ++ppu_cycle;
        goto VISIBLE_SCANLINES;
    }

    if (ppu_cycle < 257) {
        switch(ppu_cycle % 8) {
            case 1:
                // nt byte
               nt = ppu->get_mirrored_addr(ppu->v & 0xFFF);
                LOAD_SHIFT_REGISTERS(bg_latch);
                break;

            case 2:
                // nt byte
                nt = ppu->nametables[nt];
                break;

            case 3:
                // at byte
                break;

            case 4:
                // at byte
                break;

            case 5:
                // low bg
                addr_latch = (PPUCTRL_BG(ppu->ppu_ctrl) << 12) | (nt << 4) | 
                    VRAM_FINE_Y(ppu->v);
                break;

            case 6:
                // low bg
                bg_latch = ppu->ptables[addr_latch];
                break;

            case 7:
                // high bg
                addr_latch |= 8;
                break;

            case 0:
                // high bg
                bg_latch |= (ppu->ptables[addr_latch] << 8);
                if (RENDERING_ENABLED(ppu->ppu_mask)) {
                    INC_HORIZONTAL_V(ppu->v);
                    if (ppu_cycle == 256) {
                        INC_VERTICAL_V(ppu->v);
                    }
                }
                break;
        }
        bg_pixel = (((bg_high >> (15 - ppu->fine_x)) & 1) << 1) | ((bg_low >> (15 - ppu->fine_x)) & 1);
        if (PPUMASK_BG_SHOW(ppu->ppu_mask)) {
            if (ppu_cycle < 9 && PPUMASK_BG_LEFTMOST(ppu->ppu_mask) == 0) {
                pixels[pixel_count++] = colors[0];
            }else {

                pixels[pixel_count++] = colors[bg_pixel];
            }
        } else {
            pixels[pixel_count++] = colors[0];
        }
        ++ppu_cycle;
        bg_low <<= 1;
        bg_high <<= 1;
        goto VISIBLE_SCANLINES;

    } 
    // horizontal(v) = horizontal(t)
    if(RENDERING_ENABLED(ppu->ppu_mask)) {
        ppu->v = (ppu->v & HORIZONTAL_V) | (ppu->t & VERTICAL_V);
    }
    ++ppu_cycle;

SPRITE_FETCH:
    CPU_CYCLE(cpu);
    --cycles;

    if (ppu_cycle < 321) {
        switch(ppu_cycle % 8) {
            case 5:
                // low sprite tile byte
                break;

            case 6:
                // low sprite tile byte
                break;

            case 7:
                // high sprite tile byte
                break;

            case 0:
                // high sprite tile byte
                break;
        }
        ++ppu_cycle;
        goto SPRITE_FETCH;
    }

    // 321
    ++ppu_cycle;
    nt = ppu->get_mirrored_addr(ppu->v & 0xFFF);
    LOAD_SHIFT_REGISTERS(bg_latch);
    bg_low <<= 1;
    bg_high <<= 1;

TWO_TILES:
    CPU_CYCLE(cpu);
    --cycles;

    if (ppu_cycle < 337) {
        switch(ppu_cycle % 8) {
            case 1:
                // nt byte
                nt = ppu->get_mirrored_addr(ppu->v & 0xFFF);
                LOAD_SHIFT_REGISTERS(bg_latch);
                break;
            case 2:
                // nt byte
                nt = ppu->nametables[nt];
                break;

            case 3:
                // at byte
                break;
            case 4:
                // at byte
                break;

            case 5:
                // low bg
                addr_latch = (PPUCTRL_BG(ppu->ppu_ctrl) << 12) | (nt << 4) | 
                    VRAM_FINE_Y(ppu->v);
                break;

            case 6:
                // low bg
                bg_latch = ppu->ptables[addr_latch];
                break;

            case 7:
                // high bg
                addr_latch |= 8;
                break;

            case 0:
                // high bg
                bg_latch |= (ppu->ptables[addr_latch] << 8);
                if (RENDERING_ENABLED(ppu->ppu_mask)) {
                    INC_HORIZONTAL_V(ppu->v);
                }
                break;
        }
        ++ppu_cycle;
        bg_low <<= 1;
        bg_high <<= 1;
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

PRE_SPRITE_FETCH:
    CPU_CYCLE(cpu);
    --cycles;

    if (ppu_cycle < 321) {
        switch(ppu_cycle % 8) {
            case 5:
                break;

            case 6:
                break;

            case 7:
                break;

            case 0:
                if (ppu_cycle == 304) {
                    if (RENDERING_ENABLED(ppu->ppu_mask)) {
                        ppu->v = (ppu->v & VERTICAL_V) | (ppu->t & HORIZONTAL_V);
                    }
                }
                break;
        }
        ++ppu_cycle;
        goto PRE_SPRITE_FETCH;
    }

    // 321
    ++ppu_cycle;
    nt = ppu->get_mirrored_addr(ppu->v & 0xFFF);
    LOAD_SHIFT_REGISTERS(bg_latch);
    bg_high <<= 1;
    bg_low <<= 1;

PRE_TWO_TILES:
    CPU_CYCLE(cpu);
    --cycles;

    if (ppu_cycle < 337) {
        switch(ppu_cycle % 8) {
            case 1:
                // nt byte
                nt = ppu->get_mirrored_addr(ppu->v & 0xFFF);
                LOAD_SHIFT_REGISTERS(bg_latch);
                break;

            case 2:
                // nt byte
                nt = ppu->nametables[nt];
                break;

            case 3:
                // at byte
                break;

            case 4:
                // at byte
                break;

            case 5:
                // low bg
                addr_latch = (PPUCTRL_BG(ppu->ppu_ctrl) << 12) | (nt << 4) | 
                    VRAM_FINE_Y(ppu->v);
                break;

            case 6:
                // low bg
                bg_latch = ppu->ptables[addr_latch];
                break;

            case 7:
                // high bg
                addr_latch |= 8;
                break;

            case 0:
                // high bg
                bg_latch |= (ppu->ptables[addr_latch] << 8);
                if (RENDERING_ENABLED(ppu->ppu_mask)) {
                    INC_HORIZONTAL_V(ppu->v);
                }
                break;
        }
        bg_low <<= 1;
        bg_high <<= 1;
        ++ppu_cycle;
        goto PRE_TWO_TILES;
    }

    if (ppu_cycle == 339) {
        if ((frames & 1)) {
            scanline = 0;
            ppu_cycle = 0;
            ++frames;
            pixel_count = 0;
            SDL_UpdateTexture(texture, NULL, pixels, pitch);
            SDL_RenderCopy(renderer, texture, NULL, NULL);
            SDL_RenderPresent(renderer);
            joypad_events();
           
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
        joypad_events();
        // render
        goto VISIBLE_SCANLINES;
    }
    ++ppu_cycle;
    goto PRE_TWO_TILES;

}
