all:
	gcc -O2 main.c ./mos_6502/cpu_impl.c ./nes1/nes.c ./nes1/ppu.c ./nes1/mappers.c ./nes1/mappers/nrom.c ./nes1/mappers/uxrom.c utils.c  -o emu `sdl2-config --cflags --libs`; ./emu 

clean:
	rm ./emu
