
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


#define AT_X ((VRAM_X(ppu->v) / 2) % 2)
#define AT_Y ((VRAM_Y(ppu->v) / 2) % 2)

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


#define LOAD_BG_REGISTERS(_bg_latch)                    \
    bg_low = bg_low | (_bg_latch & 0xFF);               \
    bg_high = bg_high | ((_bg_latch >> 8) & 0xFF);      \
    at_low = at_low | ((at_latch & 1) ? 0xFF : 0x00);   \
    at_high = at_high | ((at_latch & 2) ? 0xFF : 0x00); \

#define UPDATE_BG_REGISTERS()   \
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
                bg_latch = ppu->ptables[addr_latch];                                \
                break;                                                              \
            case 7:                                                                 \
                addr_latch |= 8;                                                    \
                break;                                                              \
            case 0:                                                                 \
                bg_latch |= (ppu->ptables[addr_latch] << 8);                        \
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
                bg_latch = ppu->ptables[addr_latch];                                \
                break;                                                              \
            case 7:                                                                 \
                addr_latch |= 8;                                                    \
                break;                                                              \
            case 0:                                                                 \
                bg_latch |= (ppu->ptables[addr_latch] << 8);                        \
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
    int at_low = 0;
    int at_high = 0;
    int at_latch = 0;
    int pixel_mux = 0;
    int bg_palette = 0;
    unsigned char bg_pixel = 0;

    // sprites
    int oam_count = 0;
    unsigned char oam_data = 0;
    unsigned char sprite_in_range = 0;
    unsigned char sprite_zero_next = 1;
    unsigned char sprite_zero_current = 0;


VISIBLE_SCANLINES:
    CPU_CYCLE(cpu);
    --cycles;
    if (ppu_cycle == 0) {
        ++ppu_cycle;
        ppu->total_sprites = 0; // secondary oam clear
        goto VISIBLE_SCANLINES;
    }

    if (ppu_cycle < 257) {
        BACKGROUND_TILES();


        pixel_mux = PPUCTRL_SPRITE_SIZE(ppu->ppu_ctrl) ? 15 : 7; 

        /* printf("scanline - %d, oam_count - %d\n", scanline, oam_count); */
        if (ppu_cycle > 64 && oam_count < 256 && PPUSTATUS_OVERFLOW(ppu->ppu_status) == 0) {

            if (ppu_cycle & 1) {
                // cycle is odd
                if (ppu_cycle == 65) {
                    oam_count = cpu->mem[0x2003];   // evaluation starts at this oam address(sprite zero) 
                }

                oam_data = ppu->oam[oam_count];
            } else {
                // cycle is even
                // check of range
                if (sprite_in_range == 0 && scanline >= oam_data && (scanline - oam_data) <= pixel_mux) {
                    /* printf("y - %d\n", oam_data); */
                    if (sprite_zero_next == 1) {
                        sprite_zero_next = 2;
                    }
                    sprite_in_range = 4;
                    if (ppu->total_sprites == 8) {
                        PPUSTATUS_SET_OVERFLOW(ppu->ppu_status);
                        sprite_in_range = 0;
                    } else {
                        ++ppu->total_sprites;
                    }
                }

                sprite_zero_next &= 2;
                if (sprite_in_range > 0) {
                    ppu->oam2[oam_count++] = oam_data;
                    --sprite_in_range;
                } else {
                    oam_count += 4;
                }
            }

        }

        pixel_mux = 0x8000 >> ppu->fine_x;
        bg_pixel = (((bg_high & pixel_mux) > 0) << 1) | ((bg_low & pixel_mux) > 0);
        bg_palette = (((at_high & pixel_mux) > 0) << 1) | ((at_low & pixel_mux) > 0);

        if (bg_pixel == 0) {
            pixels[pixel_count] = static_pallete[ppu->palette[0]];
        } else {
            if (PPUMASK_BG_SHOW(ppu->ppu_mask)) {
                pixels[pixel_count] = static_pallete[ppu->palette[(bg_palette * 4) + bg_pixel]];
            } else {
                pixels[pixel_count] = static_pallete[ppu->palette[0]];
            }
        }
        ++pixel_count;
        ++ppu_cycle;
        UPDATE_BG_REGISTERS();
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

//-------------------------------------------------------------------------------


/* #include "../mos_6502/cpu_impl.h" */
/* #include "ppu.h" */
/* #include "nes.h" */
/* #include <stdio.h> */
/* #include <SDL2/SDL.h> */
/* #include <SDL2/SDL_events.h> */
/* #include <SDL2/SDL_pixels.h> */
/* #include <SDL2/SDL_render.h> */
/* #include <SDL2/SDL_timer.h> */
/* #include <SDL2/SDL_video.h> */

/* #define HORIZONTAL_V 0b1111101111100000 */
/* #define VERTICAL_V   0b1000010000011111 */

/* #define INC_HORIZONTAL_V(_v)                    \ */
/*     if ((_v & 0x001F) == 31){                   \ */
/*         _v &= ~0x001F;                          \ */
/*         _v ^= 0x0400;                           \ */
/*     } else {                                    \ */
/*         _v += 1;                                \ */
/*     }                                           \ */

/* #define INC_VERTICAL_V(_v)                      \ */
/*     if ((_v & 0x7000) != 0x7000){               \ */
/*         _v += 0x1000;                           \ */
/*     } else {                                    \ */
/*         _v &= ~0x7000;                          \ */
/*         y = (_v & 0x03E0) >> 5;                 \ */
/*         if (y == 29){                           \ */
/*             y = 0;                              \ */
/*             _v ^= 0x0800;                       \ */
/*         } else if (y == 31) {                   \ */
/*             y = 0;                              \ */
/*         } else {                                \ */
/*             y += 1;                             \ */
/*         }                                       \ */
/*         _v = (_v & ~0x03E0) | (y << 5);         \ */
/*     }                                           \ */

/* #define CPU_CYCLE(_cpu)                 \ */
/*     if (cycles == 0) {                  \ */
/*         ++number;                       \ */
/*         cycles = cpu_cycle(_cpu) * 3;    \ */
/*     }                                   \ */


/* #define AT_X ((VRAM_X(ppu->v) / 2) % 2) */
/* #define AT_Y ((VRAM_Y(ppu->v) / 2) % 2) */


/* void print_nametables() { */
/*     // name table 1 */ 
/*     for (int i = 0; i < 960; i++) { */
/*         if (i % 32 == 0) { */
/*             printf("\n"); */
/*         } */
/*         printf(" %d ", ppu->nametables[i]); */
/*     } */
/*     printf("----------------------------------\n"); */

/* } */

/* void print_oam2() { */
/*     for (int i = 0; i < 32; i += 4) { */
/*         printf("y - %d, tile - %d, at - %d, x - %d\n", ppu->oam2[i], ppu->oam2[i + 1], ppu->oam2[i + 2], ppu->oam2[i + 3]); */
/*     } */
/*     printf("____________________________________________\n"); */
/* } */


/* void joypad_events() { */
/*     while(SDL_PollEvent(&event)) { */
/*         if (event.type == SDL_KEYDOWN) { */
/*             switch(event.key.keysym.sym) { */
/*                 case SDLK_d: */
/*                     JOYPAD_SET(joypad1, BUTTON_A); */
/*                     break; */
/*                 case SDLK_f: */
/*                     JOYPAD_SET(joypad1, BUTTON_B); */
/*                     break; */
/*                 case SDLK_RETURN: */
/*                     JOYPAD_SET(joypad1, BUTTON_START); */
/*                     break; */
/*                 case SDLK_SPACE: */
/*                     JOYPAD_SET(joypad1, BUTTON_SELECT); */
/*                     break; */
/*                 case SDLK_DOWN: */
/*                     JOYPAD_SET(joypad1, BUTTON_DOWN); */
/*                     break; */
/*                 case SDLK_UP: */
/*                     JOYPAD_SET(joypad1, BUTTON_UP); */
/*                     break; */
/*                 case SDLK_LEFT: */
/*                     JOYPAD_SET(joypad1, BUTTON_LEFT); */
/*                     break; */
/*                 case SDLK_RIGHT: */
/*                     JOYPAD_SET(joypad1, BUTTON_RIGHT); */
/*                     break; */
/*             } */
/*         } else if(event.type == SDL_KEYUP) { */

/*             switch(event.key.keysym.sym) { */
/*                 case SDLK_d: */
/*                     JOYPAD_CLEAR(joypad1, BUTTON_A); */
/*                     break; */
/*                 case SDLK_f: */
/*                     JOYPAD_CLEAR(joypad1, BUTTON_B); */
/*                     break; */
/*                 case SDLK_RETURN: */
/*                     JOYPAD_CLEAR(joypad1, BUTTON_START); */
/*                     break; */
/*                 case SDLK_SPACE: */
/*                     JOYPAD_CLEAR(joypad1, BUTTON_SELECT); */
/*                     break; */
/*                 case SDLK_DOWN: */
/*                     JOYPAD_CLEAR(joypad1, BUTTON_DOWN); */
/*                     break; */
/*                 case SDLK_UP: */
/*                     JOYPAD_CLEAR(joypad1, BUTTON_UP); */
/*                     break; */
/*                 case SDLK_LEFT: */
/*                     JOYPAD_CLEAR(joypad1, BUTTON_LEFT); */
/*                     break; */
/*                 case SDLK_RIGHT: */
/*                     JOYPAD_CLEAR(joypad1, BUTTON_RIGHT); */
/*                     break; */
/*             } */
/*         } else if (event.type == SDL_QUIT) { */
/*             SDL_Quit(); */
/*             exit(0); */
/*         } */
/*     } */
/* } */
/* int vertical_mirroring(int addr) { */
/*     if (addr < 0x400) { */
/*         return addr; */
/*     } */
/*     if (addr < 0x800) { */
/*         return (addr & 0x3FF) + 0x400; */
/*     } */
/*     if (addr < 0xC00) { */
/*         return addr & 0x3FF; */
/*     } */

/*     // addr < 0x1000 */
/*     return (addr & 0x3FF) + 0x400; */
/* } */

/* int horizontal_mirroring(int addr) { */
/*     if (addr < 0x800) { */
/*         return addr & 0x3FF; */
/*     } */

/*     return (addr & 0x3FF) + 0x400; */
/* } */


/* #define LOAD_BG_REGISTERS(_bg_latch)                    \ */
/*     bg_low = bg_low | (_bg_latch & 0xFF);               \ */
/*     bg_high = bg_high | ((_bg_latch >> 8) & 0xFF);      \ */
/*     at_low = at_low | ((at_latch & 1) ? 0xFF : 0x00);   \ */
/*     at_high = at_high | ((at_latch & 2) ? 0xFF : 0x00); \ */

/* #define UPDATE_BG_REGISTERS()   \ */
/*     bg_low <<= 1;               \ */
/*     bg_high <<= 1;              \ */
/*     at_low <<= 1;               \ */
/*     at_high <<= 1;              \ */


/* void ppu_render(struct Cpu *cpu) { */

/*     unsigned int static_pallete[0x40] = { */

/*         0xFF808080, 0xFF003DA6, 0xFF0012B0, 0xFF440096, 0xFFA1005E, */
/*         0xFFC70028, 0xFFBA0600, 0xFF8C1700, 0xFF5C2F00, 0xFF104500, */
/*         0xFF054A00, 0xFF00472E, 0xFF004166, 0xFF000000, 0xFF050505, */
/*         0xFF050505, 0xFFC7C7C7, 0xFF0077FF, 0xFF2155FF, 0xFF8237FA, */
/*         0xFFEB2FB5, 0xFFFF2950, 0xFFFF2200, 0xFFD63200, 0xFFC46200, */
/*         0xFF358000, 0xFF058F00, 0xFF008A55, 0xFF0099CC, 0xFF212121, */
/*         0xFF090909, 0xFF090909, 0xFFFFFFFF, 0xFF0FD7FF, 0xFF69A2FF, */
/*         0xFFD480FF, 0xFFFF45F3, 0xFFFF618B, 0xFFFF8833, 0xFFFF9C12, */
/*         0xFFFABC20, 0xFF9FE30E, 0xFF2BF035, 0xFF0CF0A4, 0xFF05FBFF, */
/*         0xFF5E5E5E, 0xFF0D0D0D, 0xFF0D0D0D, 0xFFFFFFFF, 0xFFA6FCFF, */
/*         0xFFB3ECFF, 0xFFDAABEB, 0xFFFFA8F9, 0xFFFFABB3, 0xFFFFD2B0, */
/*         0xFFFFEFA6, 0xFFFFF79C, 0xFFD7E895, 0xFFA6EDAF, 0xFFA2F2DA, */
/*         0xFF99FFFC, 0xFFDDDDDD, 0xFF111111, 0xFF111111, */
/*     }; */

/*     int number = 0; */
/*     SDL_Window *window; */
/*     SDL_Renderer *renderer; */
/*     if (SDL_Init(SDL_INIT_VIDEO) < 0) { */
/*         SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Couldn't initialize SDL: %s", SDL_GetError()); */
/*         exit(3); */
/*     } */
/*     window = SDL_CreateWindow("NesBox", SDL_WINDOWPOS_CENTERED, */
/*             SDL_WINDOWPOS_CENTERED, 768, 240*3, SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE); */
/*     renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_TARGETTEXTURE | */ 
/*             SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC); */
/*     SDL_Texture *texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888, */ 
/*             SDL_TEXTUREACCESS_STREAMING, 256, 240); */
/*     unsigned int colors[4] = {0x00000000, 0xFF4f4f4f, 0xFF828282, 0xFFFFFFFF}; */
/*     unsigned int pixels[256 * 240]; */
/*     int pitch = 256 * sizeof(unsigned int); */
/*     int pixel_count = 0; */

/*     int y = 0; */
/*     int scanline = 0; */
/*     int ppu_cycle = 0; */
/*     int cycles = 21; */
/*     int frames = 0; */

/*     int bg_low = 0; */
/*     int bg_high = 0; */

/*     int bg_latch = 0; */

/*     int addr_latch = 0; */

/*     int nt = 0; */
/*     int at_low = 0; */
/*     int at_high = 0; */
/*     int at_latch = 0; */
/*     int pixel_mux = 0; */
/*     unsigned char bg_palette = 0; */
/*     unsigned char bg_pixel = 0; */
/*     unsigned char flip_h = 0; */
/*     unsigned char sprite_pixel = 0; */
/*     unsigned char sprite_palette = 0; */
/*     unsigned char sprite_priority = 0; */
/*     char pixel_type = 0;    // zero for background and 1 for sprite and 2 for sprite zero */
/*     int oam_count = 0; */
/*     unsigned char oam_data = 0; */
/*     unsigned char sprite_zero_next = 1; */
/*     unsigned char sprite_zero_current = 0; */
/*     unsigned char sprite_in_range = 0; */

/*     unsigned char sprites_attrs[8]; */
/*     unsigned char sprites_x[8]; */
/*     unsigned char sprites_low[8]; */
/*     unsigned char sprites_high[8]; */
/*     char sprites_to_render = 0; */


/* VISIBLE_SCANLINES: */
/*     CPU_CYCLE(cpu); */
/*     --cycles; */
/*     if (ppu_cycle == 0) { */
/*         ++ppu_cycle; */
/*         ppu->total_sprites = 0; // secondary oam clear */
/*         sprite_in_range = 0; */
/*         goto VISIBLE_SCANLINES; */
/*     } */

/*     if (ppu_cycle < 257) { */
/*         switch(ppu_cycle % 8) { */
/*             case 1: */
/*                 // nt byte */
/*                 nt = ppu->get_mirrored_addr(ppu->v & 0xFFF); */
/*                 LOAD_BG_REGISTERS(bg_latch); */
/*                 break; */

/*             case 2: */
/*                 // nt byte */
/*                 nt = ppu->nametables[nt]; */
/*                 break; */

/*             case 3: */
/*                 // at byte */
/*                 at_latch = ppu->get_mirrored_addr((0x23C0 | (ppu->v & 0x0C00) | ((ppu->v >> 4) & 0x38) | ((ppu->v >> 2) & 0x07)) & 0xFFF); */
/*                 break; */

/*             case 4: */
/*                 // at byte */
/*                 at_latch = ppu->nametables[at_latch]; */
/*                 at_latch = (at_latch >> (((AT_Y << 1) | AT_X) << 1)) & 3; */
/*                 break; */

/*             case 5: */
/*                 // low bg */
/*                 addr_latch = (PPUCTRL_BG(ppu->ppu_ctrl) << 12) | (nt << 4) | */ 
/*                     VRAM_FINE_Y(ppu->v); */
/*                 break; */

/*             case 6: */
/*                 // low bg */
/*                 bg_latch = ppu->ptables[addr_latch]; */
/*                 break; */

/*             case 7: */
/*                 // high bg */
/*                 addr_latch |= 8; */
/*                 break; */

/*             case 0: */
/*                 // high bg */
/*                 bg_latch |= (ppu->ptables[addr_latch] << 8); */
/*                 if (RENDERING_ENABLED(ppu->ppu_mask)) { */
/*                     INC_HORIZONTAL_V(ppu->v); */
/*                     if (ppu_cycle == 256) { */
/*                         INC_VERTICAL_V(ppu->v); */
/*                     } */
/*                 } */
/*                 break; */
/*         } */

/*         pixel_mux = PPUCTRL_SPRITE_SIZE(ppu->ppu_ctrl) ? 15 : 7; */ 

/*         /1* printf("scanline - %d, oam_count - %d\n", scanline, oam_count); *1/ */
/*         if (ppu_cycle > 64 && oam_count < 256 && PPUSTATUS_OVERFLOW(ppu->ppu_status) == 0) { */

/*             if (ppu_cycle & 1) { */
/*                 // cycle is odd */
/*                 if (ppu_cycle == 65) { */
/*                     oam_count = cpu->mem[0x2003];   // evaluation starts at this oam address(sprite zero) */ 
/*                 } */

/*                 oam_data = ppu->oam[oam_count]; */
/*             } else { */
/*                 // cycle is even */
/*                 // check of range */
/*                 if (sprite_in_range == 0 && scanline >= oam_data && (scanline - oam_data) <= pixel_mux) { */
/*                     /1* printf("y - %d\n", oam_data); *1/ */
/*                     if (sprite_zero_next == 1) { */
/*                         sprite_zero_next = 2; */
/*                     } */
/*                     sprite_in_range = 4; */
/*                     if (ppu->total_sprites == 8) { */
/*                         PPUSTATUS_SET_OVERFLOW(ppu->ppu_status); */
/*                         sprite_in_range = 0; */
/*                     } else { */
/*                         ++ppu->total_sprites; */
/*                     } */
/*                 } */

/*                 sprite_zero_next &= 2; */
/*                 if (sprite_in_range > 0) { */
/*                     ppu->oam2[oam_count++] = oam_data; */
/*                     --sprite_in_range; */
/*                 } else { */
/*                     oam_count += 4; */
/*                 } */
/*             } */

/*         } */

/*         sprites_to_render = 0; */
/*         pixel_type = 0; */
/*         while (sprites_to_render < ppu->total_sprites) { */
/*             printf("scanline - %d, ppu_cycle - %d\n", scanline, ppu_cycle); */
/*             flip_h = sprites_attrs[sprites_to_render] & 0b01000000; */
/*             if (sprites_x[sprites_to_render] == 1) { */
/*                 if (flip_h) { */
/*                     sprite_pixel = ((sprites_high[sprites_to_render] & 1) << 1) | (sprites_low[sprites_to_render] & 1); */
/*                     sprites_low[sprites_to_render] >>= 1; */
/*                     sprites_high[sprites_to_render] >>= 1; */
/*                 } else { */
/*                     sprite_pixel = ((sprites_high[sprites_to_render] >> 7) << 1) | (sprites_low[sprites_to_render] >> 7); */
/*                     sprites_low[sprites_to_render] <<= 1; */
/*                     sprites_high[sprites_to_render] <<= 1; */
/*                 } */

/*                     /1* printf("scanline - %d, ppu_cycle - %d\n", scanline, ppu_cycle); *1/ */
/*                 if (sprite_pixel > 0) { */
/*                     pixel_type = 1; */
/*                     sprite_palette = sprites_attrs[sprites_to_render] & 3; */
/*                     sprite_priority = sprites_attrs[sprites_to_render] & 0b00100000; */
/*                     break; */
/*                 } */
/*             } else { */
/*                 --sprites_x[sprites_to_render]; */
/*             } */
/*             ++sprites_to_render; */
/*         } */
        

/*         pixel_mux = 0x8000 >> ppu->fine_x; */
/*         bg_pixel = (((bg_high & pixel_mux) > 0) << 1) | ((bg_low & pixel_mux) > 0); */
/*         bg_palette = (((at_high & pixel_mux) > 0) << 1) | ((at_low & pixel_mux) > 0); */

/*         if (bg_pixel == 0) { */
/*             if (PPUMASK_SPRITE_SHOW(ppu->ppu_mask) && pixel_type == 1) { */
/*                 // show sprite */
/*                 pixels[pixel_count] = static_pallete[ppu->palette[0x10 + (sprite_palette * 4) + sprite_pixel]]; */
/*             } else { */
/*                 pixels[pixel_count] = static_pallete[ppu->palette[0]]; */
/*             } */
/*         } else { */
/*             if (PPUMASK_BG_SHOW(ppu->ppu_mask)) { */
/*                 if (pixel_type == 1 && PPUMASK_SPRITE_SHOW(ppu->ppu_mask) && sprite_priority == 0) { */
/*                     // show sprite */
/*                     pixels[pixel_count] = static_pallete[ppu->palette[0x10 + (sprite_palette * 4) + sprite_pixel]]; */
/*                 } else { */
/*                     pixels[pixel_count] = static_pallete[ppu->palette[(bg_palette * 4) + bg_pixel]]; */
/*                 } */
/*             } else { */
/*                 if (PPUMASK_SPRITE_SHOW(ppu->ppu_mask) && pixel_type == 1) { */
/*                     // show sprite */
/*                     pixels[pixel_count] = static_pallete[ppu->palette[0x10 + (sprite_palette * 4) + sprite_pixel]]; */
/*                 } else { */
/*                     pixels[pixel_count] = static_pallete[ppu->palette[0]]; */
/*                 } */
/*             } */
/*         } */
/*         ++pixel_count; */
/*         ++ppu_cycle; */
/*         UPDATE_BG_REGISTERS(); */
/*         goto VISIBLE_SCANLINES; */

/*     } */ 
/*     // horizontal(v) = horizontal(t) */
/*     if(RENDERING_ENABLED(ppu->ppu_mask)) { */
/*         ppu->v = (ppu->v & HORIZONTAL_V) | (ppu->t & VERTICAL_V); */
/*     } */
/*     print_oam2(); */
/*     printf("scanline - %d, sprites - %d\n", scanline, ppu->total_sprites); */
/*     /1* printf("scanline - %d, sprites - %d\n", scanline, ppu->total_sprites); *1/ */
/*     cpu->mem[0x2003] = 0; */

/*     /1* oam_count = ppu->total_sprites * 4; *1/ */
/*     sprites_to_render = 0; */
/*     /1* printf("total_sprites - %d\n", ppu->total_sprites); *1/ */
/* SPRITE_FETCH: */
/*     CPU_CYCLE(cpu); */
/*     --cycles; */

/*     if (ppu_cycle < 321) { */
/*         cpu->mem[0x2003] = 0; */
/*         if (sprites_to_render < ppu->total_sprites) { */
/*             switch(ppu_cycle % 8) { */

/*                 case 6: */
/*                     // low sprite tile byte */
/*                     addr_latch = (PPUCTRL_SPRITE(ppu->ppu_ctrl) << 12) | */ 
/*                                     ppu->oam2[oam_count + 1] |ppu->oam2[oam_count]; */

/*                     sprites_low[sprites_to_render] = ppu->ptables[addr_latch]; */
/*                     sprites_attrs[sprites_to_render] = ppu->oam2[oam_count + 2]; */
/*                     sprites_x[sprites_to_render] = ppu->oam2[oam_count + 3] + 1; */
/*                     break; */

/*                 case 0: */
/*                     // high sprite tile byte */
/*                     sprites_high[sprites_to_render] = ppu->ptables[addr_latch | 8]; */
/*                     /1* sprites_to_render += 4; *1/ */
/*                     ++sprites_to_render; */
/*                     oam_count += 4; */
/*                     break; */
/*             } */
/*         } */
/*         ++ppu_cycle; */
/*         goto SPRITE_FETCH; */
/*     /1* printf("x - %d, at - %d\n", sprites_x[0], sprites_attrs[0]); *1/ */

/*     // 321 */
/*     ++ppu_cycle; */
/*     addr_latch = ppu->get_mirrored_addr(ppu->v & 0xFFF); */
/*     UPDATE_BG_REGISTERS(); */

/* TWO_TILES: */
/*     CPU_CYCLE(cpu); */
/*     --cycles; */

/*     if (ppu_cycle < 337) { */
/*         switch(ppu_cycle % 8) { */
/*             case 1: */
/*                 // nt byte */
/*                 addr_latch = ppu->get_mirrored_addr(ppu->v & 0xFFF); */
/*                 LOAD_BG_REGISTERS(bg_latch); */
/*                 break; */
/*             case 2: */
/*                 // nt byte */
/*                 nt = ppu->nametables[addr_latch]; */
/*                 break; */

/*             case 3: */
/*                 // at byte */
/*                 addr_latch = ppu->get_mirrored_addr((0x23C0 | (ppu->v & 0x0C00) | ((ppu->v >> 4) & 0x38) | ((ppu->v >> 2) & 0x07)) & 0xFFF); */
/*                 break; */
/*             case 4: */
/*                 // at byte */
/*                 at_latch = ppu->nametables[addr_latch]; */
/*                 at_latch = (at_latch >> (((AT_Y << 1) | AT_X) << 1)) & 3; */
/*                 break; */

/*             case 5: */
/*                 // low bg */
/*                 addr_latch = (PPUCTRL_BG(ppu->ppu_ctrl) << 12) | (nt << 4) | */ 
/*                     VRAM_FINE_Y(ppu->v); */
/*                 break; */

/*             case 6: */
/*                 // low bg */
/*                 bg_latch = ppu->ptables[addr_latch]; */
/*                 break; */

/*             case 7: */
/*                 // high bg */
/*                 addr_latch |= 8; */
/*                 break; */

/*             case 0: */
/*                 // high bg */
/*                 bg_latch |= (ppu->ptables[addr_latch] << 8); */
/*                 if (RENDERING_ENABLED(ppu->ppu_mask)) { */
/*                     INC_HORIZONTAL_V(ppu->v); */
/*                 } */
/*                 break; */
/*         } */
/*         ++ppu_cycle; */
/*         UPDATE_BG_REGISTERS(); */
/*         goto TWO_TILES; */
/*     } */

/*     if (ppu_cycle == 340) { */
/*         // next scanline */
/*         ++scanline; */
/*         ppu_cycle = 0; */
/*         if (scanline == 240) { */
/*             goto INVISIBLE_SCANLINES; */
/*         } */
/*         goto VISIBLE_SCANLINES; */
/*     } */
/*     ++ppu_cycle; */
/*     goto TWO_TILES; */

/* INVISIBLE_SCANLINES: */
/*     CPU_CYCLE(cpu); */
/*     --cycles; */
/*     if (scanline == 241 && ppu_cycle == 1) { */
/*         // set vblank */
/*         PPUSTATUS_SET_VBLANK(ppu->ppu_status); */
/*         if (PPUCTRL_NMI(ppu->ppu_ctrl)) { */
/*             cpu->nmi = 1; */

/*         } */
/*     } else { */
/*         if (ppu_cycle == 340) { */
/*             // inc scanline */
/*             ++scanline; */
/*             ppu_cycle = 0; */
/*             if (scanline == 261) { */
/*                 goto PRE_RENDER_SCANLINE; */
/*             } */
/*             goto INVISIBLE_SCANLINES; */
/*         } */
/*     } */
/*     ++ppu_cycle; */
/*     goto INVISIBLE_SCANLINES; */

/* PRE_RENDER_SCANLINE: */
/*     CPU_CYCLE(cpu); */
/*     --cycles; */

/*     if (ppu_cycle == 0) { */
/*         ++ppu_cycle; */
/*         goto PRE_RENDER_SCANLINE; */
/*     } */

/*     if (ppu_cycle == 1) { */
/*         // clear vblank sprite and overflow */
/*         ppu->ppu_status &= 0b00011111; */
/*         ++ppu_cycle; */
/*         goto PRE_RENDER_SCANLINE; */
/*     } */

/*     if (ppu_cycle < 257) { */
/*         switch(ppu_cycle % 8) { */
/*             case 0: */
/*                 // inc horizontal(v) */
/*                 if (RENDERING_ENABLED(ppu->ppu_mask)) { */
/*                     INC_HORIZONTAL_V(ppu->v); */
/*                     if (ppu_cycle == 256) { */
/*                         INC_VERTICAL_V(ppu->v); */
/*                     } */
/*                 } */
/*                 break; */
/*         } */
/*         ++ppu_cycle; */
/*         goto PRE_RENDER_SCANLINE; */
/*     } */


/*     // horizontal (v) = horizontal(t) */
/*     if(RENDERING_ENABLED(ppu->ppu_mask)) { */
/*         ppu->v = (ppu->v & HORIZONTAL_V) | (ppu->t & VERTICAL_V); */
/*     } */
/*     ++ppu_cycle; */

/* PRE_SPRITE_FETCH: */
/*     CPU_CYCLE(cpu); */
/*     --cycles; */

/*     if (ppu_cycle < 321) { */
/*         switch(ppu_cycle % 8) { */
/*             case 5: */
/*                 break; */

/*             case 6: */
/*                 break; */

/*             case 7: */
/*                 break; */

/*             case 0: */
/*                 if (ppu_cycle == 304) { */
/*                     if (RENDERING_ENABLED(ppu->ppu_mask)) { */
/*                         ppu->v = (ppu->v & VERTICAL_V) | (ppu->t & HORIZONTAL_V); */
/*                     } */
/*                 } */
/*                 break; */
/*         } */
/*         ++ppu_cycle; */
/*         goto PRE_SPRITE_FETCH; */
/*     } */

/*     // 321 */
/*     ++ppu_cycle; */
/*     addr_latch = ppu->get_mirrored_addr(ppu->v & 0xFFF); */
/*     UPDATE_BG_REGISTERS(); */
    

/* PRE_TWO_TILES: */
/*     CPU_CYCLE(cpu); */
/*     --cycles; */

/*     if (ppu_cycle < 337) { */
/*         switch(ppu_cycle % 8) { */
/*             case 1: */
/*                 // nt byte */
/*                 addr_latch = ppu->get_mirrored_addr(ppu->v & 0xFFF); */
/*                 LOAD_BG_REGISTERS(bg_latch); */
/*                 break; */

/*             case 2: */
/*                 // nt byte */
/*                 nt = ppu->nametables[addr_latch]; */
/*                 break; */

/*             case 3: */
/*                 // at byte */
/*                 addr_latch = ppu->get_mirrored_addr((0x23C0 | (ppu->v & 0x0C00) | ((ppu->v >> 4) & 0x38) | ((ppu->v >> 2) & 0x07)) & 0xFFF); */
/*                 break; */

/*             case 4: */
/*                 // at byte */
/*                 at_latch = ppu->nametables[addr_latch]; */
/*                 at_latch = (at_latch >> (((AT_Y << 1) | AT_X) << 1)) & 3; */
/*                 break; */

/*             case 5: */
/*                 // low bg */
/*                 addr_latch = (PPUCTRL_BG(ppu->ppu_ctrl) << 12) | (nt << 4) | */ 
/*                     VRAM_FINE_Y(ppu->v); */
/*                 break; */

/*             case 6: */
/*                 // low bg */
/*                 bg_latch = ppu->ptables[addr_latch]; */
/*                 break; */

/*             case 7: */
/*                 // high bg */
/*                 addr_latch |= 8; */
/*                 break; */

/*             case 0: */
/*                 // high bg */
/*                 bg_latch |= (ppu->ptables[addr_latch] << 8); */
/*                 if (RENDERING_ENABLED(ppu->ppu_mask)) { */
/*                     INC_HORIZONTAL_V(ppu->v); */
/*                 } */
/*                 break; */
/*         } */
/*         UPDATE_BG_REGISTERS(); */
/*         ++ppu_cycle; */
/*         goto PRE_TWO_TILES; */
/*     } */

/*     if (ppu_cycle == 339) { */
/*         if (RENDERING_ENABLED(ppu->ppu_mask) && (frames & 1)) { */
/*             scanline = 0; */
/*             ppu_cycle = 0; */
/*             ++frames; */
/*             pixel_count = 0; */
/*             SDL_UpdateTexture(texture, NULL, pixels, pitch); */
/*             SDL_RenderCopy(renderer, texture, NULL, NULL); */
/*             SDL_RenderPresent(renderer); */
/*             joypad_events(); */
           
/*             // render */
/*             goto VISIBLE_SCANLINES; */
/*         } */
/*     } */

/*     if (ppu_cycle == 340) { */
/*         scanline = 0; */
/*         ppu_cycle = 0; */
/*         ++frames; */
/*         pixel_count = 0; */
/*         SDL_UpdateTexture(texture, NULL, pixels, pitch); */
/*         SDL_RenderCopy(renderer, texture, NULL, NULL); */
/*         SDL_RenderPresent(renderer); */
/*         joypad_events(); */
/*         // render */
/*         goto VISIBLE_SCANLINES; */
/*     } */
/*     ++ppu_cycle; */
/*     goto PRE_TWO_TILES; */

/* } */
/* } */
