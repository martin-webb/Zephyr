#include "cartridge.h"
#include "gameboy.h"
#include "lcdgl.h"
#include "pixel.h"
#include "sound/coreaudio.h"
#include "utils/os.h"

#include <GLUT/glut.h>

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define EMULATOR_NAME "Zephyr"

#define TARGET_WINDOW_WIDTH_DEFAULT 1024
#define WINDOW_SCALE_FACTOR (TARGET_WINDOW_WIDTH_DEFAULT * 1.0 / LCD_WIDTH)

GameBoy gameBoy;
Pixel frameBuffer[LCD_WIDTH * LCD_HEIGHT];
AudioSampleBuffer audioSampleBuffer;

bool fast = false;

void runGBWithGLUT(int value)
{
  glutTimerFunc(17, runGBWithGLUT, value);
  gbRunNFrames(&gameBoy, &audioSampleBuffer, (fast) ? 4 : 1);

  glutPostRedisplay();
}

void reshape(int width, int height)
{
  glViewport(0, 0, width, height);

  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  gluOrtho2D(0, LCD_WIDTH, 0, LCD_HEIGHT);

  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity();
}

void keyPressed(unsigned char key, int x, int y)
{
  switch (key) {
    case 120: // X
      gameBoy.joypadController._a = true;
      break;
    case 97: // A
      gameBoy.joypadController._b = true;
      break;
    case 13: // Return
      gameBoy.joypadController._start = true;
      break;
    case 9: // Tab
      gameBoy.joypadController._select = true;
      break;
    case 32: // Space
      fast = true;
      break;
    case 49: // 1 - Toggle sound channel 1
      gameBoy.soundController.channel1Master = !gameBoy.soundController.channel1Master;
      break;
    case 50: // 2 - Toggle sound channel 2
      gameBoy.soundController.channel2Master = !gameBoy.soundController.channel2Master;
      break;
    case 51: // 3 - Toggle sound channel 3
      gameBoy.soundController.channel3Master = !gameBoy.soundController.channel3Master;
      break;
    case 52: // 4 - Toggle sound channel 4
      gameBoy.soundController.channel4Master = !gameBoy.soundController.channel4Master;
      break;
  }
}

void keyUp(unsigned char key, int x, int y)
{
  switch (key) {
    case 120: // X
      gameBoy.joypadController._a = false;
      break;
    case 97: // A
      gameBoy.joypadController._b = false;
      break;
    case 13: // Return
      gameBoy.joypadController._start = false;
      break;
    case 9: // Tab
      gameBoy.joypadController._select = false;
      break;
    case 32: // Space
      fast = false;
      break;
  }
}

void specialKeyPressed(int key, int x, int y)
{
  switch (key) {
    case GLUT_KEY_UP:
      gameBoy.joypadController._up = true;
      break;
    case GLUT_KEY_DOWN:
      gameBoy.joypadController._down = true;
      break;
    case GLUT_KEY_LEFT:
      gameBoy.joypadController._left = true;
      break;
    case GLUT_KEY_RIGHT:
      gameBoy.joypadController._right = true;
      break;
  }
}

void specialKeyUp(int key, int x, int y)
{
  switch (key) {
    case GLUT_KEY_UP:
      gameBoy.joypadController._up = false;
      break;
    case GLUT_KEY_DOWN:
      gameBoy.joypadController._down = false;
      break;
    case GLUT_KEY_LEFT:
      gameBoy.joypadController._left = false;
      break;
    case GLUT_KEY_RIGHT:
      gameBoy.joypadController._right = false;
      break;
  }
}

GameBoyType getGameBoyType(int argc, const char* argv[])
{
  if (argc >= 3) {
    if (strcmp(argv[2], "--gb") == 0) {
      return GB;
    } else if (strcmp(argv[2], "--cgb") == 0) {
      return CGB;
    } else {
      return GB;
    }
  } else {
    return GB;
  }
}

int main(int argc, const char* argv[])
{
  if (argc < 2) {
    printf("Usage: %s PATH_TO_ROM\n", argv[0]);
    return 1;
  }

  const char* romFilename = basename(argv[1]);
  const GameBoyType gameBoyType = getGameBoyType(argc, argv);

  // Load all cartridge data
  uint8_t* cartridgeData = cartridgeLoadData(argv[1]);
  if (cartridgeData == NULL) {
    fprintf(stderr, "Failed to read cartridge from '%s'\n", argv[1]);
    exit(EXIT_FAILURE);
  }

  sampleBufferInitialise(&audioSampleBuffer, 512 * 10); // CoreAudio requests buffers of 512 samples, so ten times that

  struct GBAudioInfo* audioInfo = initCoreAudioPlayback(&audioSampleBuffer);

  gbInitialise(&gameBoy, gameBoyType, cartridgeData, frameBuffer, romFilename);

  glutInit(&argc, (char**)argv);
  glutInitDisplayMode(GLUT_DOUBLE | GLUT_DEPTH | GLUT_RGBA);
  glutInitWindowSize(LCD_WIDTH * WINDOW_SCALE_FACTOR, LCD_HEIGHT * WINDOW_SCALE_FACTOR);
  glutInitWindowPosition(
    (glutGet(GLUT_SCREEN_WIDTH) - (LCD_WIDTH * WINDOW_SCALE_FACTOR)) / 2 - 100,
    (glutGet(GLUT_SCREEN_HEIGHT) - (LCD_HEIGHT * WINDOW_SCALE_FACTOR)) / 2
  );

  // Prepare window title
  const char* windowTitlePrefix = EMULATOR_NAME " - ";
  char* windowTitle = (char*)malloc(strlen(windowTitlePrefix) + strlen(romFilename) + 1);
  strcpy(windowTitle, windowTitlePrefix);
  strcat(windowTitle, romFilename);

  glutCreateWindow(windowTitle);

  glutReshapeFunc(reshape);
  glutDisplayFunc(lcdGLDrawScreen);
  glutTimerFunc(0, runGBWithGLUT, 0);

  glutKeyboardFunc(keyPressed);
  glutKeyboardUpFunc(keyUp);
  glutSpecialFunc(specialKeyPressed);
  glutSpecialUpFunc(specialKeyUp);

  lcdGLInit();
  lcdGLInitPixelVerticesArray();

  glutMainLoop();

  gbFinalise(&gameBoy);

  free((void*)audioInfo);
  free((void*)windowTitle);
  free((void*)romFilename);
  free(cartridgeData);

  return 0;
}