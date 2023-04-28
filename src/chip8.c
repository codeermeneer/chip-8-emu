#include "chip8.h"
#include "raylib.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

unsigned char chip8_fontset[80] =
{
  0xF0, 0x90, 0x90, 0x90, 0xF0, // 0
  0x20, 0x60, 0x20, 0x20, 0x70, // 1
  0xF0, 0x10, 0xF0, 0x80, 0xF0, // 2
  0xF0, 0x10, 0xF0, 0x10, 0xF0, // 3
  0x90, 0x90, 0xF0, 0x10, 0x10, // 4
  0xF0, 0x80, 0xF0, 0x10, 0xF0, // 5
  0xF0, 0x80, 0xF0, 0x90, 0xF0, // 6
  0xF0, 0x10, 0x20, 0x40, 0x40, // 7
  0xF0, 0x90, 0xF0, 0x90, 0xF0, // 8
  0xF0, 0x90, 0xF0, 0x10, 0xF0, // 9
  0xF0, 0x90, 0xF0, 0x90, 0x90, // A
  0xE0, 0x90, 0xE0, 0x90, 0xE0, // B
  0xF0, 0x80, 0x80, 0x80, 0xF0, // C
  0xE0, 0x90, 0x90, 0x90, 0xE0, // D
  0xF0, 0x80, 0xF0, 0x80, 0xF0, // E
  0xF0, 0x80, 0xF0, 0x80, 0x80  // F
};

void init(chip8 *c8)
{
  c8->pc = 0x200;
  c8->opcode = 0;
  c8->I = 0;
  c8->sp = 0;

  memset(c8->pixels, 0, sizeof(c8->pixels));
  memset(c8->stack, 0, sizeof(c8->stack));
  memset(c8->V, 0, sizeof(c8->V));
  memset(c8->memory, 0, sizeof(c8->memory));

  for(int i = 0; i < 80; ++i)
    c8->memory[i] = chip8_fontset[i];

  c8->delay_timer = 0;
  c8->sound_timer = 0;

  c8->draw_flag = 0;
}

int load_rom(chip8 *c8, FILE *rom)
{
  fseek(rom, 0, SEEK_END);
  int size = ftell(rom);
  fseek(rom, 0, SEEK_SET);

  unsigned int count = fread(c8->memory+512, sizeof(unsigned char), size, rom);
  fclose(rom);

  return count;
}

void emulate_cycle(chip8 *c8)
{
  // fetch
  c8->opcode = c8->memory[c8->pc] << 8 | c8->memory[c8->pc+1];
  c8->pc += 2;
  //printf("0x%04x\n", c8->opcode);

  unsigned char X = (c8->opcode & 0x0F00) >> 8;
  unsigned char Y = (c8->opcode & 0x00F0) >> 4;
  unsigned char N = c8->opcode & 0x000F;
  unsigned char NN = c8->opcode & 0x00FF;
  unsigned short NNN = c8->opcode & 0x0FFF;

  switch (c8->opcode & 0xF000) {
    case 0x0000:
      switch (c8->opcode & 0x000F) {
        case 0x0000:
          memset(c8->pixels, 0, sizeof(c8->pixels));
          c8->draw_flag = 1;
          break;
        case 0x000E:
          c8->pc = c8->stack[--c8->sp];
          c8->pc += 2;
          break;
        default:
          printf("Unknown opcode\n");
      }
      break;
    case 0x1000:
      c8->pc = NNN;
      break;
    case 0x2000:
      c8->stack[c8->sp++] = c8->pc-2;
      c8->pc = NNN;
      break;
    case 0x3000:
      if (c8->V[X] == NN)
        c8->pc += 2;
      break;
    case 0x4000:
      if (c8->V[X] != NN)
        c8->pc += 2;
      break;
    case 0x5000:
      if (c8->V[X] == c8->V[Y])
        c8->pc += 2;
      break;
    case 0x6000:
      c8->V[X] = NN;
      break;
    case 0x7000:
      c8->V[X] += NN;
      break;
    case 0x8000:
      switch (c8->opcode & 0x000F) {
        case 0x0000:
          c8->V[X] = c8->V[Y];
          break;
        case 0x0001:
          c8->V[X] |= c8->V[Y];
          c8->V[0xF] = 0;
          break;
        case 0x0002:
          c8->V[X] &= c8->V[Y];
          c8->V[0xF] = 0;
          break;
        case 0x0003:
          c8->V[X] ^= c8->V[Y];
          c8->V[0xF] = 0;
          break;
        case 0x0004:
          unsigned int sum = c8->V[X] + c8->V[Y];
          c8->V[X] = sum;

          if (sum & 0x100)
            c8->V[0xF] = 1;
          else
            c8->V[0xF] = 0;
          break;
        case 0x0005:
          unsigned int result = c8->V[X] - c8->V[Y];
          c8->V[X] = result;

          if (result & 0x100)
            c8->V[0xF] = 0;
          else
            c8->V[0xF] = 1;
          break;
        case 0x0006:
          unsigned char V_F = c8->V[X] & 1;
          c8->V[X] = c8->V[Y];
          c8->V[X] >>= 1;
          c8->V[0xF] = V_F;
          break;
        case 0x0007:
          result = c8->V[Y] - c8->V[X];
          c8->V[X] = result;

          if (result & 0x100)
            c8->V[0xF] = 0;
          else
            c8->V[0xF] = 1;
          break;
        case 0x000E:
          V_F = (c8->V[X] & 0x80) >> 7;
          c8->V[X] = c8->V[Y];
          c8->V[X] <<= 1;
          c8->V[0xF] = V_F;
          break;
        default:
          printf("Unknown opcode\n");
      }
      break;
    case 0x9000:
      if (c8->V[X] != c8->V[Y])
        c8->pc += 2;
      break;
    case 0xA000:
      c8->I = NNN;
      break;
    case 0xB000:
      c8->pc = c8->V[0] + NNN;
      break;
    case 0xC000:
      c8->V[X] = rand() & NN;
      break;
    case 0xD000:
      unsigned char x = c8->V[X] % 64;
      unsigned char y = c8->V[Y] % 32;

      unsigned char pixel;

      c8->V[0xF] = 0;

      for (int row = 0; row < N; row++) {
        pixel = c8->memory[c8->I + row];

        for (int col = 0; col < 8; col++) {
          if((pixel & (0x80 >> col)) != 0) {
            if(c8->pixels[x+col][y+row] == 1) {
              c8->pixels[x+col][y+row] = 0x0;
              c8->V[0xF] = 0x0001;
            } else {
              c8->pixels[x+col][y+row] = 0x1;
            }
          }
          if (x + col == 64)
            break;
        }
        if (y + row == 32)
          break;
      }
      c8->draw_flag = 1;
      break;
    case 0xE000:
      switch(c8->opcode & 0x000F) {
        case 0x000E:
          if (c8->key[c8->V[X]])
            c8->pc += 2;
          break;
        case 0x0001:
          if (!c8->key[c8->V[X]])
            c8->pc += 2;
          break;
        default:
      }
      break;
    case 0xF000:
      switch(c8->opcode & 0x00FF) {
        case 0x0007:
          c8->V[X] = c8->delay_timer;
          break;
        case 0x000A:
          // TODO wait till key is released
          c8->pc -= 2;
          for (int i = 0; i < 0xF; i++) {
            if (c8->key[i]) {
              c8->V[X] = (char) i;
              c8->pc += 2;
              break;
            }
          }
          break;
        case 0x0015:
          c8->delay_timer = c8->V[X];
          break;
        case 0x0018:
          c8->sound_timer = c8->V[X];
          break;
        case 0x001E:
          c8->I += c8->V[X];
          break;
        case 0x0029:
          c8->I = c8->V[X] * 0x5;
          break;
        case 0x0033:
          c8->memory[c8->I] = c8->V[X] / 100;
          c8->memory[c8->I+1] = (c8->V[X] % 100) / 10;
          c8->memory[c8->I+2] = (c8->V[X] % 10);
          break;
        case 0x0055:
          //TODO bc_test.ch8 indicates somethings wrong with 0xFX55 and 0xF65
          memcpy(&c8->memory[c8->I++], c8->V, X+1);
          break;
        case 0x0065:
          //TODO bc_test.ch8 indicates somethings wrong with 0xFX55 and 0xF65
          memcpy(c8->V, &c8->memory[c8->I++], X+1);
          break;
        default:
      }
      break;
    default:
      printf("Unknown opcode\n");

  }
}

void check_input(chip8 *c8)
{
  c8->key[0x1] = IsKeyDown(KEY_ONE);
  c8->key[0x2] = IsKeyDown(KEY_TWO);
  c8->key[0x3] = IsKeyDown(KEY_THREE);
  c8->key[0xC] = IsKeyDown(KEY_FOUR);
  c8->key[0x4] = IsKeyDown(KEY_Q);
  c8->key[0x5] = IsKeyDown(KEY_W);
  c8->key[0x6] = IsKeyDown(KEY_E);
  c8->key[0xD] = IsKeyDown(KEY_R);
  c8->key[0x7] = IsKeyDown(KEY_A);
  c8->key[0x8] = IsKeyDown(KEY_S);
  c8->key[0x9] = IsKeyDown(KEY_D);
  c8->key[0xE] = IsKeyDown(KEY_F);
  c8->key[0xA] = IsKeyDown(KEY_Z);
  c8->key[0x0] = IsKeyDown(KEY_X);
  c8->key[0xB] = IsKeyDown(KEY_C);
  c8->key[0xF] = IsKeyDown(KEY_V);
}

void update_timers(chip8 *c8)
{
  if (c8->delay_timer != 0)
    c8->delay_timer--;
  if (c8->sound_timer != 0)
    c8->sound_timer--;
}
