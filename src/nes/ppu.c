#include <SDL2/SDL.h>
#include <SDL2/SDL_events.h>
#include <SDL2/SDL_pixels.h>
#include <SDL2/SDL_render.h>
#include <SDL2/SDL_timer.h>
#include <SDL2/SDL_video.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "ppu.h"
#include "nes.h"

static int sprite_zero_x = 0;
static int sprite_zero_next = 0;
static int sprite_zero_current = 0;

#define HORIZONTAL_V 0b1111101111100000
#define VERTICAL_V   0b1000010000011111

#define AT_X ((VRAM_X(ppu->v) / 2) % 2)
#define AT_Y ((VRAM_Y(ppu->v) / 2) % 2)

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

void print_pallete() {
    for (int i = 0; i < 0x20; i++) {
        if (i % 10 == 0) {
            printf("\n");
        }

        printf(" 0x%X ", ppu->palette[i]);
    } 
}

void print_pixels(unsigned int *pixels) {
    for (int i = 0; i < 256 * 240; i++) {
        if (i % 256 == 0) {
            printf("\n");
        }
        printf(" %d ", pixels[i]);
    }
}

void print_oam() {
    for (int i = 0; i < 256; i += 4) {
        printf("y - %d, tile - %d, at - %d, x - %d\n", ppu->oam[i], ppu->oam[i + 1], ppu->oam[i + 2], ppu->oam[i + 3]);
    }
    printf("_______________________________\n");
}

void display_pixels(int pt, unsigned int *pixels) {
    unsigned int colors[4] = {0x00000000, 0xFF4f4f4f, 0xFF828282, 0xFFFFFFFF};
    int row = 0;
    int col = 0;
    int nt = 0;
    int pixel_count = 0;
    int plane1 = 0;
    int plane0 = 0;
    
    while (row < 30) {
        nt = 0x1000 + (ppu->nametables[(32 * row) + col] * 16);
        for (int i = 0; i < 8; i++) {
            plane0 = ppu->ptables[nt + i];
            plane1 = ppu->ptables[(nt + i) | 8];
            for (int j = 7; j >= 0; j--) {
                pixels[(256 * ((row*8) + i)) + (col * 8) + (7 - j)] = colors[(((plane1 >> j) & 1)<<1) | ((plane0 >> j) & 1)]; 
                pixel_count++;
            }
        }
        ++col;
        if (col == 32) {
           ++row; 
           col = 0;
        }
    }
}

void display_single_tile(int tile, unsigned int *pixels) {
    unsigned int colors[4] = {0x00000000, 0xFF4f4f4f, 0xFF828282, 0xFFFFFFFF};
    int nt = tile * 16;
    int plane0 = 0;
    int plane1 = 0;
    for (int i = 0; i < 8; i++) {
        plane0 = ppu->ptables[nt + i];
        plane1 = ppu->ptables[(nt + i) | 8];
        for (int j = 7; j >= 0; j--) {
            pixels[(256 * i) + (7 - j)] = colors[(((plane1 >> j) & 1)<<1)|((plane0 >>j) & 1)];
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

void evaluate_sprites(int scanline, int oam_addr) {
    int temp = oam_addr;
    int sprite_size = PPUCTRL_SPRITE_SIZE(ppu->ppu_ctrl) ? 15 : 7;
    /* printf("oam_addr - %d\n", oam_addr); */
    while (oam_addr <= 252) {
        if (scanline >= ppu->oam[oam_addr] && (scanline - ppu->oam[oam_addr]) <= sprite_size) {
            if (oam_addr == temp) {
                sprite_zero_next = 1;
                /* printf("true %d, - %d\n", scanline - 1, ppu->oam[oam_addr]); */
                /* printf("sprite_zero_y %d sprite_zero_x %d\n", ppu->oam[oam_addr], ppu->oam[oam_addr + 3]); */
            }
            if (ppu->total_sprites == 8) {
                // set sprite overflow
                PPUSTATUS_SET_OVERFLOW(ppu->ppu_status);
                return;
            }
            ppu->oam2[ppu->total_sprites * 4] = scanline - ppu->oam[oam_addr];
            ppu->oam2[(ppu->total_sprites * 4) + 1] = ppu->oam[oam_addr + 1];
            ppu->oam2[(ppu->total_sprites * 4) + 2] = ppu->oam[oam_addr + 2];
            ppu->oam2[(ppu->total_sprites * 4) + 3] = ppu->oam[oam_addr + 3];

            ++ppu->total_sprites;
        }
        oam_addr += 4;
    }
    /* printf("eval total_sprites - %d\n", ppu->total_sprites); */
}

int render_sprites(unsigned char *pixels) {
    /* printf("total_sprites - %d\n", ppu->total_sprites); */
    int x;
    int attr;
    int plane0;
    int plane1;
    int flip_y;
    int j = 0;
    int pixel;
    int flip_x;
    int sprite_size = PPUCTRL_SPRITE_SIZE(ppu->ppu_ctrl) ? 15 : 7;
    ppu->total_sprites *= 4;
    sprite_zero_x = 0;
    int sprite_zero = 0;
    for (int i = 0; i < ppu->total_sprites; i += 4) {
        x = ppu->oam2[i + 3] + 1;
        attr = ppu->oam2[i + 2];
        flip_x = (attr & 0b01000000);
        flip_y = (attr & 0b10000000) ? (7 - ppu->oam2[i]) : ppu->oam2[i];
        int sprite_flip = attr & 0b10000000;
        if (sprite_size == 15) {
            plane0 = ((ppu->oam2[i + 1] & 1) << 12) | (((ppu->oam2[i + 1] & 0b11111110) | (ppu->oam2[i] >> 3)) << 4) | 
                (sprite_flip ? (7 - ppu->oam2[i] & 7) : (ppu->oam2[i] & 7));
        } else {
            plane0 = (PPUCTRL_SPRITE(ppu->ppu_ctrl) << 12) | (ppu->oam2[i + 1] << 4) | flip_y;
        }
        plane1 = ppu->ptables[plane0 | 8];
        plane0 = ppu->ptables[plane0];

        if (i == 0 && sprite_zero_next){ 
            /* printf("x - %d, y - %d\n", x, ppu->oam2[0]); */
            sprite_zero_x = x;
        }
        while (j < 8) {
            if (x > 256) {
                j = 0;
                break;
            }
            if (flip_x) {
                pixel = (((plane1 >> j) & 1) << 1) | ((plane0 >> j) & 1);
            } else {
                pixel = (((plane1 >> (7 - j)) & 1) << 1) | ((plane0 >> (7 - j)) & 1);
            }
            if (pixel > 0) {
                    /* printf("true 1\n"); */
                if (sprite_zero_next && (i == 0)) {
                    /* if (x >= 88 && x <= 95) { */
                    /*     printf("true %d\n", j ); */
                    /* } */
                    sprite_zero = (sprite_zero) | (1 << j);
                }
                if (pixels[x] == 128) {
                    pixels[x] = ((attr & 3) << 2) | pixel | (attr & 0b00100000);
                }
            }
            x += 1;
            ++j;
        }
        j = 0;

    }
    
    ppu->total_sprites = 0;
    return sprite_zero;
}

void ppu_render(struct Cpu *cpu) {
    int number = 0;
    unsigned char sprite_pixels[257];
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
    /* SDL_Event event; */
    unsigned int colors[4] = {0x00000000, 0xFF4f4f4f, 0xFF828282, 0xFFFFFFFF};
    
    int total_cycles = 7;
    int frames = 0;
    int scanlines = 261;
    int ppu_cycle = 0;
    int nametable_addr_latch = 0;
    int nametable_byte_latch = 0;
    int nametable_row = 0;
    int nametable_column = 0;
    int attribute_byte_latch = 0;
    int pattern_table_addr = 0;
    int sprite_zero = 0;
    // for background rendering
    int low_bg_tiles = 0;
    int high_bg_tiles = 0;
    int tile_latch = 0;


    int attribute_byte1 = 0;
    int attribute_byte2 = 0;

    int cpu_cycles = 21;
    int fetch_nametable = 2;
    int fetch_attribute = 4;
    int fetch_low_bg = 6;
    int fetch_high_bg = 8;
    int y = 0;
    // init sprite pixel array
    while (y <= 256) {
        sprite_pixels[y] = 128;
        ++y;
    }
    unsigned int pixels[256 * 240];
    int pixel_count = 0;
    int bg_pixel = 0;
    unsigned char sprite_pixel = 0;
    int pitch = 256 * sizeof(unsigned int);
    unsigned char oam_dma_byte = 0;
    int fetching_high_tile = 0;
/* PPU_CYCLE: */
    while (1) {
        if (cpu_cycles == 0){
            cpu_cycles = cpu_cycle(cpu);
            ++number;
            if (ppu->oam_dma == 1) {
                if ((ppu->total_cycles & 1) == 1) {
                    cpu_cycles = (cpu_cycles + 514) * 3;
                } else {
                    cpu_cycles = (cpu_cycles + 513) * 3;
                }
                ppu->oam_dma = 0;
            } else {
                ppu->total_cycles += cpu_cycles;
                cpu_cycles *= 3;
            }
        }
        --cpu_cycles;

        if (scanlines < 240) {
            // scanlines to render
            if (ppu_cycle == 0) {
                ++ppu_cycle;
                continue;
            } else if(ppu_cycle == 338) {
                // nt byte fetch
                ++ppu_cycle;
                continue;
            } else if (ppu_cycle == 340) {
                ++scanlines;
                ppu_cycle = 0;
                fetch_nametable = 2;
                fetch_attribute = 4;
                fetch_low_bg = 6;
                fetch_high_bg = 8;
                
                continue;
            } else if (ppu_cycle == fetch_nametable) {
                nametable_byte_latch = ppu->nametables[ppu->get_mirrored_addr(ppu->v & 0xFFF)];
                fetch_nametable += 8;

            } else if (ppu_cycle == fetch_attribute) {
                fetch_attribute += 8;
                attribute_byte_latch = ppu->nametables[ ppu->get_mirrored_addr((0x23C0 | (ppu->v & 0x0C00) | ((ppu->v >> 4) & 0x38) | ((ppu->v >> 2) & 0x07)) & 0xFFF)];
                attribute_byte_latch = (attribute_byte_latch >> (((AT_Y << 1) | AT_X) << 1)) & 3;

            } else if (ppu_cycle == fetch_low_bg) {
                tile_latch = ppu->ptables[((PPUCTRL_BG(ppu->ppu_ctrl) << 12) | (nametable_byte_latch << 4) | (VRAM_FINE_Y(ppu->v)))];
                fetch_low_bg += 8;

            }else if (ppu_cycle == 257) {
                // For now we evaluate sprites in this cycle. {Temperory}
                evaluate_sprites(scanlines, cpu->mem[0x2003]);
                cpu->mem[0x2003] = 0;
                fetch_nametable = 322;
                fetch_attribute = 324;
                fetch_low_bg = 326;
                fetch_high_bg = 328;
                if (RENDERING_ENABLED(ppu->ppu_mask)) {
                    ppu->v = (ppu->v & HORIZONTAL_V) | (ppu->t & VERTICAL_V);
                }
                ++ppu_cycle;
                continue;
            }

            // we render each pixel
            if (ppu_cycle < 257) {
                bg_pixel = (((high_bg_tiles >> (15 - ppu->fine_x)) & 1) << 1) | ((low_bg_tiles >> (15 - ppu->fine_x)) & 1);
                sprite_pixel = sprite_pixels[ppu_cycle];
                if (PPUMASK_BG_SHOW(ppu->ppu_mask) && (bg_pixel > 0)) {
                    if (PPUMASK_SPRITE_SHOW(ppu->ppu_mask) == 0 || sprite_pixel == 128) {
                        if (ppu_cycle < 9 && (PPUMASK_BG_LEFTMOST(ppu->ppu_mask) == 0)) {
                            // show backdrop
                            pixels[pixel_count] = static_pallete[ppu->palette[0]];
                        } else {
                            // show bg
                            pixels[pixel_count] = static_pallete[ppu->palette[(attribute_byte1 * 4) + bg_pixel]];
                        }
                    } else {
                    
                        // TODO Leftmost masking

                        if ((PPUSTATUS_ZEROHIT(ppu->ppu_status)==0) && sprite_zero_current && (ppu_cycle < 256) && ppu_cycle >= sprite_zero_x && (ppu_cycle - sprite_zero_x) <= 7) {
                            if (((sprite_zero >> (ppu_cycle - sprite_zero_x))&1)) {
                                if (ppu_cycle > 8) {
                                    PPUSTATUS_SET_ZEROHIT(ppu->ppu_status);
                                } else {
                                    if (PPUMASK_BG_LEFTMOST(ppu->ppu_status) && PPUMASK_SPRITE_LEFTMOST(ppu->ppu_status)) {
                                        PPUSTATUS_SET_ZEROHIT(ppu->ppu_status);
                                    }
                                }
                            }
                        }

                        // both are enable
                        if (sprite_pixel != 128 && (sprite_pixel & 0b00100000) == 0) {
                            // show sprite pixel
                            if (ppu_cycle < 9 && (PPUMASK_SPRITE_LEFTMOST(ppu->ppu_mask) == 0)) {
                                if (PPUMASK_BG_LEFTMOST(ppu->ppu_mask)) {
                                    // show bg
                                    pixels[pixel_count] = static_pallete[ppu->palette[(attribute_byte1 * 4) + bg_pixel]];
                                } else {
                                    // show backdrop
                                    pixels[pixel_count] = static_pallete[ppu->palette[0]];
                                }
                            } else {
                                pixels[pixel_count] = static_pallete[ppu->palette[0x10 + (sprite_pixel & 0b00001100) + (sprite_pixel & 3)]];
                            }
                        } else {
                            // show bg
                            if (ppu_cycle < 9 && (PPUMASK_BG_LEFTMOST(ppu->ppu_mask) == 0)) {
                                if (PPUMASK_SPRITE_LEFTMOST(ppu->ppu_mask) && sprite_pixel != 128) {
                                    // show sprite
                                    pixels[pixel_count] = static_pallete[ppu->palette[0x10 + (sprite_pixel & 0b00001100) + (sprite_pixel & 3)]];
                                } else {
                                    // show backdrop
                                    pixels[pixel_count] = static_pallete[ppu->palette[0]];
                                }
                            } else {
                                pixels[pixel_count] = static_pallete[ppu->palette[(attribute_byte1 * 4) + bg_pixel]];
                            }
                        }


                    }
                } else {
                    // bg is disable or bg is 0
                    if (PPUMASK_SPRITE_SHOW(ppu->ppu_mask)) {
                        if (sprite_pixel != 128) {
                            // show sprite
                            pixels[pixel_count] = static_pallete[ppu->palette[0x10 + (sprite_pixel & 0b00001100) + (sprite_pixel & 3)]];
                        } else {
                            // show backdrop
                            pixels[pixel_count] = static_pallete[ppu->palette[0]];
                        }
                    } else {
                        if (PPUMASK_BG_SHOW(ppu->ppu_mask)) {
                            // show backdrop
                            pixels[pixel_count] = static_pallete[ppu->palette[(attribute_byte1 * 4) + bg_pixel]];
                        } else {
                            // rendering is disabled
                            if ((ppu->v & 0x3FFF) >= 0x3F00 && (ppu->v & 0x3FFF) <= 0x3FFF) {
                                pixels[pixel_count] = static_pallete[ppu->palette[ppu->v & 0x1F]];
                            }else {
                                pixels[pixel_count] = static_pallete[ppu->palette[0]];
                            }
                        }
                    }
                }
                
                sprite_pixels[ppu_cycle] = 128;
                ++pixel_count;
            }
            if (ppu_cycle < 337) {
                low_bg_tiles <<= 1;
                high_bg_tiles <<= 1;
            }
            if (ppu_cycle == fetch_high_bg) {
                fetch_high_bg += 8;
                high_bg_tiles |= ppu->ptables[((PPUCTRL_BG(ppu->ppu_ctrl) << 12) | (nametable_byte_latch << 4) | (VRAM_FINE_Y(ppu->v)) | 8)];
                low_bg_tiles |= tile_latch;
                attribute_byte1 = attribute_byte2;
                attribute_byte2 = attribute_byte_latch;
                if (RENDERING_ENABLED(ppu->ppu_mask)) {
                    INC_HORIZONTAL_V(ppu->v);
                    if (ppu_cycle == 256) {
                        INC_VERTICAL_V(ppu->v);
                    }
                }
                ++ppu_cycle;
                continue;
            }

            if (ppu_cycle == 320) {
                if (RENDERING_ENABLED(ppu->ppu_mask)) {
                    sprite_zero = render_sprites(sprite_pixels);
                }
                sprite_zero_current = sprite_zero_next;
                cpu->mem[0x2003] = 0;
            }
            
            ++ppu_cycle;
            continue;
        }

        if (scanlines < 261) {
            // vblank
            if (scanlines == 241) {
                // TODO: Handle nmi suppression correctly
                if (ppu_cycle == 1) {
                    // set vblank flag
                    PPUSTATUS_SET_VBLANK(ppu->ppu_status);
                    if (PPUCTRL_NMI(ppu->ppu_ctrl)) {
                        cpu->nmi = 1;
                    }
                }
            }
            if (ppu_cycle == 340) {
                // change scanline
                ppu_cycle = 0;
                ++scanlines;
                fetch_nametable = 2;
                fetch_attribute = 4;
                fetch_low_bg = 6;
                fetch_high_bg = 8;
                continue;
            }
            ++ppu_cycle;
            continue;
        }
        // pre render scanline
        if (ppu_cycle == 1) {
            // clear vblank, sprite hit, and overflow
            low_bg_tiles = 0;
            high_bg_tiles = 0;
            ppu->ppu_status &= 0b00011111;
        
        } else if (ppu_cycle == 337) {
            ++ppu_cycle;
            continue;
        } else if (ppu_cycle == 338) {
            ++ppu_cycle;
            continue;
        } else if (ppu_cycle == 339) {
            // on odd frames tick 340 will be escaped of pre render scanline
            if (frames & 1) {   // frame is odd
                scanlines = 0;
                ppu_cycle = 0;
                ++frames;
                
                fetch_nametable = 2;
                fetch_attribute = 4;
                fetch_low_bg = 6;
                fetch_high_bg = 8;
                pixel_count = 0;
                SDL_UpdateTexture(texture, NULL, pixels, 256 * (sizeof(unsigned int)));
                SDL_RenderCopy(renderer, texture, NULL, NULL);
                SDL_RenderPresent(renderer);
                /* joypad_events(); */
                continue;
            }

        } else if (ppu_cycle == 340) {
            scanlines = 0;
            ppu_cycle = 0;
            ++frames;
            fetch_nametable = 2;
            fetch_attribute = 4;
            fetch_low_bg = 6;
            fetch_high_bg = 8;
            pixel_count = 0;
            SDL_UpdateTexture(texture, NULL, pixels, pitch);
            SDL_RenderCopy(renderer, texture, NULL, NULL);
            SDL_RenderPresent(renderer);
            /* joypad_events(); */

            continue;
        } else if (ppu_cycle == fetch_nametable) {
            fetch_nametable += 8;
            if (ppu_cycle > 320) {
                // fetch nametable
                nametable_addr_latch = ppu->get_mirrored_addr(ppu->v & 0xFFF);
                nametable_byte_latch = ppu->nametables[ppu->get_mirrored_addr(ppu->v & 0xFFF)];
            }
        } else if (ppu_cycle == fetch_attribute) {
            fetch_attribute += 8;
            if (ppu_cycle > 320) {
                // fetch attribute
                attribute_byte_latch = ppu->nametables[ ppu->get_mirrored_addr((0x23C0 | (ppu->v & 0x0C00) | ((ppu->v >> 4) & 0x38) | ((ppu->v >> 2) & 0x07)) & 0xFFF)];
                /* attribute_byte_latch = ppu->nametables[(nametable_addr_latch < 960 ? (960 + attribute_byte_latch) : (1984 + attribute_byte_latch))]; */
                attribute_byte_latch = (attribute_byte_latch >> (((AT_Y << 1) | AT_X) << 1)) & 3;
            }

        } else if (ppu_cycle == fetch_low_bg) {
            fetch_low_bg += 8;
            if (ppu_cycle > 320) {
                // fetch low tile
                tile_latch = ppu->ptables[((PPUCTRL_BG(ppu->ppu_ctrl) << 12) | (nametable_byte_latch << 4) | (VRAM_FINE_Y(ppu->v)))];
            }

        } else if (ppu_cycle == fetch_high_bg) {
            fetch_high_bg += 8;
            if (ppu_cycle > 320) {
                // fetch high tile
                low_bg_tiles <<= 1;
                high_bg_tiles <<= 1;
                high_bg_tiles |= ppu->ptables[((PPUCTRL_BG(ppu->ppu_ctrl) << 12) | (nametable_byte_latch << 4) | (VRAM_FINE_Y(ppu->v)) | 8)];
                low_bg_tiles |= tile_latch;
                attribute_byte1 = attribute_byte2;
                attribute_byte2 = attribute_byte_latch;
            }
            if (RENDERING_ENABLED(ppu->ppu_mask)) {
                INC_HORIZONTAL_V(ppu->v);
                if (ppu_cycle == 256) {
                    INC_VERTICAL_V(ppu->v);
                }
            }
            ++ppu_cycle;
            continue;

        } else if (ppu_cycle == 257) {
            cpu->mem[0x2003] = 0;
            fetch_nametable = 322;
            fetch_attribute = 324;
            fetch_low_bg = 326;
            fetch_high_bg = 328;
            if (RENDERING_ENABLED(ppu->ppu_mask)) {
                ppu->v = (ppu->v & HORIZONTAL_V) | (ppu->t & VERTICAL_V);
            }
        } else if (ppu_cycle == 304) {
            if (RENDERING_ENABLED(ppu->ppu_mask)) {
                ppu->v = (ppu->v & VERTICAL_V) | (ppu->t & HORIZONTAL_V);
            }
        }
        if (ppu_cycle == 320) {
            sprite_zero = render_sprites(sprite_pixels);
            cpu->mem[0x2003] = 0;
            y = 0;
            while (y <= 256) {
                sprite_pixels[y] = 128;
                ++y;
            }
            sprite_zero_next = sprite_zero_current = 0;
            ++ppu_cycle;
            continue;
        }
        if (ppu_cycle > 320) {

            low_bg_tiles <<= 1;
            high_bg_tiles <<= 1;
        }
        ++ppu_cycle;
        continue;

    }
}

