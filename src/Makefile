all:
	gcc -O2 main.c ./mos_6502/cpu_impl.c ./nes/nes.c ./nes/controllers/joypad.c ./nes/ppu.c ./nes/mappers.c ./nes/mappers/nrom.c ./nes/mappers/uxrom.c ./nes/mappers/gxrom.c ./nes/mappers/mmc1.c utils.c  -o emu `sdl2-config --cflags --libs`; ./emu 

clean:
	rm ./emu
