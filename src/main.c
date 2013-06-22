#include "cartridge.h"
#include "cpu.h"
#include "gameboy.h"
#include "interrupts.h"
#include "joypad.h"
#include "lcd.h"
#include "lcdgl.h"
#include "logging.h"
#include "speed.h"
#include "timercontroller.h"
#include "timing.h"
#include "utils/os.h"

#include <GLUT/glut.h>

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#define TARGET_WINDOW_WIDTH_DEFAULT 1024
#define WINDOW_SCALE_FACTOR (TARGET_WINDOW_WIDTH_DEFAULT * 1.0 / LCD_WIDTH)

CPU cpu;
JoypadController joypadController;
LCDController lcdController;
TimerController timerController;
InterruptController interruptController;
MemoryController memoryController;
GameBoyType gameBoyType;
SpeedMode speedMode;

uint8_t frameBuffer[LCD_WIDTH * LCD_HEIGHT];

bool fast = false;

void runGBWithGLUT(int value)
{
  glutTimerFunc(17, runGBWithGLUT, value);

  gbRunAtLeastNCycles(
    &cpu,
    &memoryController,
    &lcdController,
    &timerController,
    &interruptController,
    gameBoyType,
    speedMode,
    CPU_MIN_CYCLES_PER_SET * (fast ? 4 : 1)
  );

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
      joypadController._a = true;
      break;
    case 97: // A
      joypadController._b = true;
      break;
    case 13: // Return
      joypadController._start = true;
      break;
    case 9: // Tab
      joypadController._select = true;
      break;
    case 32: // Space
      fast = true;
      break;
  }
}

void keyUp(unsigned char key, int x, int y)
{
  switch (key) {
    case 120: // X
      joypadController._a = false;
      break;
    case 97: // A
      joypadController._b = false;
      break;
    case 13: // Return
      joypadController._start = false;
      break;
    case 9: // Tab
      joypadController._select = false;
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
      joypadController._up = true;
      break;
    case GLUT_KEY_DOWN:
      joypadController._down = true;
      break;
    case GLUT_KEY_LEFT:
      joypadController._left = true;
      break;
    case GLUT_KEY_RIGHT:
      joypadController._right = true;
      break;
  }
}

void specialKeyUp(int key, int x, int y)
{
  switch (key) {
    case GLUT_KEY_UP:
      joypadController._up = false;
      break;
    case GLUT_KEY_DOWN:
      joypadController._down = false;
      break;
    case GLUT_KEY_LEFT:
      joypadController._left = false;
      break;
    case GLUT_KEY_RIGHT:
      joypadController._right = false;
      break;
  }
}

int main(int argc, const char* argv[])
{
  if (argc != 2) {
    printf("Usage: %s PATH_TO_ROM\n", argv[0]);
    return 1;
  }

  lcdGLInitPixelVerticesArray();

  // Load all cartridge data
  uint8_t* cartridgeData = cartridgeLoadData(argv[1]);
  if (cartridgeData == NULL) {
    fprintf(stderr, "Failed to read cartridge from '%s'\n", argv[1]);
    exit(EXIT_FAILURE);
  }

  uint8_t memory[1024 * 32] = {0};

  uint8_t cartridgeType = cartridgeGetType(cartridgeData);
  printf("Cartridge Type: 0x%02X - %s\n", cartridgeType, CartridgeTypeToString(cartridgeType));

  uint8_t ramSize = cartridgeData[RAM_SIZE_ADDRESS];
  uint32_t externalRAMSizeBytes = RAMSizeInBytes(ramSize);
  printf("RAM size: 0x%02X - %s\n", ramSize, RAMSizeToString(ramSize));

  initJoypadController(&joypadController);
  initLCDController(&lcdController, &(memory[0]), &(memory[0xFE00 - CARTRIDGE_SIZE]), &(frameBuffer[0]));
  initTimerController(&timerController);

  interruptController.f = 0;
  interruptController.e = 0;

  const char* romFilename = basename(argv[1]);

  memoryController = InitMemoryController(
    cartridgeType,
    memory,
    cartridgeData,
    &joypadController,
    &lcdController,
    &timerController,
    &interruptController,
    externalRAMSizeBytes,
    romFilename
  );

  gameBoyType = GB;
  speedMode = NORMAL;

  GameBoyType gameType = gbGetGameType(cartridgeData);

  const char* gameTitle = cartridgeGetGameTitle(cartridgeData);
  printf("Title: %s\n", gameTitle);
  free((void*)gameTitle);

  printf("Game Type: %s\n", (gameType == GB) ? "GB" : (gameType == CGB) ? "CGB" : "SGB");

  uint8_t romSize = cartridgeData[ROM_SIZE_ADDRESS];
  printf("ROM size: 0x%02X - %s\n", romSize, ROMSizeToString(romSize));

  uint8_t destinationCode = cartridgeData[DESTINATION_CODE_ADDRESS];
  printf("Destination Code: 0x%02X - %s\n", destinationCode, DestinationCodeToString(destinationCode));

  cpuReset(&cpu, &memoryController, GB);
  cpuPrintState(&cpu);

  glutInit(&argc, (char**)argv);
  glutInitDisplayMode(GLUT_DOUBLE | GLUT_DEPTH | GLUT_RGBA);
  glutInitWindowSize(LCD_WIDTH * WINDOW_SCALE_FACTOR, LCD_HEIGHT * WINDOW_SCALE_FACTOR);
  glutInitWindowPosition(
    (glutGet(GLUT_SCREEN_WIDTH) - (LCD_WIDTH * WINDOW_SCALE_FACTOR)) / 2 - 100,
    (glutGet(GLUT_SCREEN_HEIGHT) - (LCD_HEIGHT * WINDOW_SCALE_FACTOR)) / 2
  );

  glutCreateWindow("GBEmu1");

  glutReshapeFunc(reshape);
  glutDisplayFunc(lcdGLDrawScreen);
  glutTimerFunc(0, runGBWithGLUT, 0);

  lcdGLInit();

  glutKeyboardFunc(keyPressed);
  glutKeyboardUpFunc(keyUp);
  glutSpecialFunc(specialKeyPressed);
  glutSpecialUpFunc(specialKeyUp);

  glutMainLoop();

  free((void*)romFilename);
  free(cartridgeData);

  return 0;
}