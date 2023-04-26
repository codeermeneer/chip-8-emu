chip-8: main.c
	cc -o chip-8 main.c chip8.c -lraylib -lGL -lm -lpthread -ldl -lrt -lX11
