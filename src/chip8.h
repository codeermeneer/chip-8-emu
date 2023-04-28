#ifndef CHIP8_H_   /* Include guard */
#define CHIP8_H_

typedef struct {
  unsigned short opcode;
  unsigned char memory[4096];
  unsigned char V[16];
  unsigned short I;
  unsigned short pc;
  unsigned char pixels[64][32];
  unsigned char delay_timer;
  unsigned char sound_timer;
  unsigned short stack[16];
  unsigned short sp;
  unsigned char key[16];
  int draw_flag;
} chip8;

void init(chip8 *c8);

int load_rom(chip8 *c8, char *rom);

void emulate_cycle(chip8 *c8);

void check_input(chip8 *c8);

void update_timers(chip8 *c8);

#endif
