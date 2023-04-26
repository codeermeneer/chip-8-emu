#include "raylib.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

typedef struct {
  unsigned short opcode;
  unsigned char memory[4096];
  unsigned char V[16];
  unsigned short I;
  unsigned short pc;
  unsigned char pixels[64*32];
  unsigned char delay_timer;
  unsigned char sound_timer;
  unsigned short stack[16];
  unsigned short sp;
  unsigned char key[16];
  int draw_flag;
} chip8;

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

int load_rom(chip8 *c8, char *rom)
{
  FILE *file;

  if ((file  = fopen(rom, "r")) == NULL) {
    printf("Cannot open file %s\n", rom);
    return -1;
  }

  fseek(file, 0, SEEK_END);
  int size = ftell(file);
  fseek(file, 0, SEEK_SET);

  unsigned int count = fread(c8->memory+512, sizeof(unsigned char), size, file);
  fclose(file);

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
          if (c8->V[Y] > (0xFF - c8->V[X]))
            c8->V[0xF] = 1;
          else
            c8->V[0xF] = 0;

          c8->V[X] += c8->V[Y];
          break;
        case 0x0005:
          if (c8->V[Y] > c8->V[X])
            c8->V[0xF] = 1;
          else
            c8->V[0xF] = 0;

          c8->V[X] -= c8->V[Y];
          break;
        case 0x0006:
          c8->V[0xF] = c8->V[X] & 1;
          c8->V[X] = c8->V[Y];
          c8->V[X] >>= 1;
          break;
        case 0x0007:
          if (c8->V[X] > c8->V[Y])
            c8->V[0xF] = 1;
          else
            c8->V[0xF] = 0;

          c8->V[X] = c8->V[Y] - c8->V[X];
          break;
        case 0x000E:
          c8->V[0xF] = c8->V[X] & 0x80;
          c8->V[X] = c8->V[Y];
          c8->V[X] <<= 1;
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
      for (int y_line = 0; y_line < N; y_line++) {
        pixel = c8->memory[c8->I + y_line];
        for (int x_line = 0; x_line < 8; x_line++) {
          if((pixel & (0x80 >> x_line)) != 0) {
            int i = (x + x_line) + ((y + y_line) * 64) % 2048;
            if(c8->pixels[i] == 0)
              c8->V[0xF] = 0x0001;
              c8->pixels[i] ^= 0x0001;
            }
              if (x + x_line == 64)
                  break;

          }
          if (y + y_line == 32)
              break;
      }
      c8->draw_flag = true;
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
          //printf("V[X]: %x\n", c8->V[X]);
          //printf("%d %d %d\n", c8->memory[c8->I], c8->memory[c8->I+1], c8->memory[c8->I+2]);
          break;
        case 0x0055:
          memcpy(&c8->memory[c8->I++], c8->V, X+1);
          break;
        case 0x0065:
          memcpy(c8->V, &c8->memory[c8->I++], X+1);
          break;
        default:
      }
      break;
    default:
      printf("Unknown opcode\n");

  }
}

void draw(chip8 *c8)
{
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

//------------------------------------------------------------------------------------
// Program main entry point
//------------------------------------------------------------------------------------
int main(int argc, char* argv[])
{
  // Initialization
  //--------------------------------------------------------------------------------------
  const int screenWidth = 640;
  const int screenHeight = 320;

  InitWindow(screenWidth, screenHeight, "chip-8");

  SetTargetFPS(1000);            // Set our game to run at 240 frames-per-second
  SetTraceLogLevel(LOG_WARNING);

  int cpu_freq = 500;
  int timer_freq = 60;
  double last_cpu_update = 0;
  double last_timer_update = 0;

  chip8 c8;
  init(&c8);
  int rom_size = load_rom(&c8, argv[1]);
  if (rom_size <= 0) {
    return 0;
  }

  /*for (int i = 512; i < 512+rom_size; i+=2) {
    printf("%02x%02x\n", c8.memory[i], c8.memory[i+1]);
  }*/

  int width = 64;
  int height = 32;

  Color *pixels = (Color *) malloc(width * height * sizeof(Color));

  // Load pixels data into an image structure and create texture
  Image checkedIm = {
    .data = pixels,             // We can assign pixels directly to data
    .width = width,
    .height = height,
    .format = PIXELFORMAT_UNCOMPRESSED_R8G8B8A8,
    .mipmaps = 1
  };

  for (int y = 0; y < height; y++) {
    for (int x = 0; x < width; x++) {
      pixels[y*width + x] = RAYWHITE;
    }
  }

  Texture2D checked = LoadTextureFromImage(checkedIm);

  //--------------------------------------------------------------------------------------

  // Main game loop
  while (!WindowShouldClose())  // Detect window close button or ESC key
  {

    // Update
    //----------------------------------------------------------------------------------

    check_input(&c8);

    double delta = GetTime() - last_cpu_update;
    if (delta >= 1.0/cpu_freq) {
      emulate_cycle(&c8);
      //printf("CPU Update! delta: %f\n", delta);
      last_cpu_update += delta;
    }

    delta = GetTime() - last_timer_update;
    if (delta >= 1.0/timer_freq) {
      update_timers(&c8);
      //printf("Timer Update! delta: %f\n", delta);
      last_timer_update += delta;
    }
    //----------------------------------------------------------------------------------

    // Draw
    //----------------------------------------------------------------------------------
    BeginDrawing();
    // ClearBackground(RAYWHITE);

    if (c8.draw_flag) {
      for (int y = 0; y < 32; y++) {
        for (int x = 0; x < 64; x++) {
          if (c8.pixels[y*64+x]) {
            pixels[y*64 + x] = RAYWHITE;
          } else {
            pixels[y*64 + x] = BLACK;
          }
        }
      }
      checked = LoadTextureFromImage(checkedIm);
      draw(&c8);
    }
    DrawTextureEx(checked, (Vector2) {0, 0}, 0.0f, 10.0f, WHITE);

    DrawFPS(0, 0);


    EndDrawing();
    //----------------------------------------------------------------------------------
  }

  // De-Initialization
  //--------------------------------------------------------------------------------------
  UnloadImage(checkedIm);       // Unload CPU (RAM) image data (pixels)
  UnloadTexture(checked);

  CloseWindow();                // Close window and OpenGL context
  //--------------------------------------------------------------------------------------

  return 0;
}
