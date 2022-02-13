#include <SDL2/SDL.h>
#include "../nes.h"
#include "controllers.h"

unsigned char joypad1 = 0;
unsigned char joypad2 = 0;
unsigned char joypad1_read_count = 0;

#define BUTTON_A        0b00000001;
#define BUTTON_B        0b00000010;
#define BUTTON_SELECT   0b00000100;
#define BUTTON_START    0b00001000;
#define BUTTON_UP       0b00010000;
#define BUTTON_DOWN     0b00100000;
#define BUTTON_LEFT     0b01000000;
#define BUTTON_RIGHT    0b10000000;

void joypad1_events() {
    while(SDL_PollEvent(&event)) {
        if (event.type == SDL_KEYDOWN) {
            switch(event.key.keysym.sym) {
                case SDLK_d:
                    JOYPAD_SET(joypad1, BUTTON_B);
                    break;
                case SDLK_f:
                    JOYPAD_SET(joypad1, BUTTON_A);
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
                    JOYPAD_CLEAR(joypad1, BUTTON_B);
                    break;
                case SDLK_f:
                    JOYPAD_CLEAR(joypad1, BUTTON_A);
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
