#include "chip8.h"
#include "raylib.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

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

  SetTargetFPS(1000);
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

  InitAudioDevice();
  Sound beep = LoadSound("resources/beep.wav");

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
          if (c8.pixels[x][y]) {
            pixels[y*64 + x] = RAYWHITE;
          } else {
            pixels[y*64 + x] = BLACK;
          }
        }
      }
      checked = LoadTextureFromImage(checkedIm);
      c8.draw_flag = 0;
    }
    DrawTextureEx(checked, (Vector2) {0, 0}, 0.0f, 10.0f, WHITE);

    //DrawFPS(0, 0);


    EndDrawing();
    //----------------------------------------------------------------------------------

    if (c8.sound_timer > 0 && !IsSoundPlaying(beep))
      PlaySound(beep);
    else if (c8.sound_timer == 0 && IsSoundPlaying(beep))
      StopSound(beep);
  }

  // De-Initialization
  //--------------------------------------------------------------------------------------
  UnloadImage(checkedIm);       // Unload CPU (RAM) image data (pixels)
  UnloadTexture(checked);

  UnloadSound(beep);

  CloseAudioDevice();
  CloseWindow();                // Close window and OpenGL context
  //--------------------------------------------------------------------------------------

  return 0;
}
