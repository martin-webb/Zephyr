#include "cartridge.h"
#include "displaylink.h"
#include "gameboy.h"
#include "lcdgl.h"
#include "logging.h"
#include "pixel.h"
#include "sound/coreaudio.h"
#include "utils/os.h"

#include <GLFW/glfw3.h>

#include <OpenGL/gl.h>

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define EMULATOR_NAME "Zephyr"

#define TARGET_WINDOW_WIDTH_DEFAULT 1024
#define WINDOW_SCALE_FACTOR (TARGET_WINDOW_WIDTH_DEFAULT * 1.0 / LCD_WIDTH)


struct UserData
{
  GameBoy* gameBoy;
  Pixel* frameBuffer;
};


static void refreshCallback(GLFWwindow* window)
{
  struct UserData* userData = glfwGetWindowUserPointer(window);

  lcdGLDrawScreen(userData->frameBuffer);
  glfwSwapBuffers(window);
}


static void setViewportAndProjection(GLFWwindow* window, int width, int height)
{
  glViewport(0, 0, width, height);

  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  glOrtho(0, LCD_WIDTH, 0, LCD_HEIGHT, -1, 1);

  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity();
}


static void sizeCallback(GLFWwindow* window, int width, int height)
{
  setViewportAndProjection(window, width, height);
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


static void errorCallback(int errorCode, const char* description)
{
  error("GLFW error: %s (%d)\n", description, errorCode);
}


static void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
  struct UserData* userData = glfwGetWindowUserPointer(window);
  GameBoy* gameBoy = userData->gameBoy;

  switch (key) {
    case GLFW_KEY_X: {
      gameBoy->joypadController._a = (action != 0);
      break;
    }
    case GLFW_KEY_A: {
      gameBoy->joypadController._b = (action != 0);
      break;
    }
    case GLFW_KEY_ENTER: {
      gameBoy->joypadController._start = (action != 0);
      break;
    }
    case GLFW_KEY_TAB: {
      gameBoy->joypadController._select = (action != 0);
      break;
    }
    case GLFW_KEY_UP: {
      gameBoy->joypadController._up = (action != 0);
      break;
    }
    case GLFW_KEY_DOWN: {
      gameBoy->joypadController._down = (action != 0);
      break;
    }
    case GLFW_KEY_LEFT: {
      gameBoy->joypadController._left = (action != 0);
      break;
    }
    case GLFW_KEY_RIGHT: {
      gameBoy->joypadController._right = (action != 0);
      break;
    }
    case GLFW_KEY_1: { // Toggle sound channel 1
      if (action == 1) {
        gameBoy->soundController.channel1Master = !gameBoy->soundController.channel1Master;
      }
      break;
    }
    case GLFW_KEY_2: { // Toggle sound channel 2
      if (action == 1) {
        gameBoy->soundController.channel2Master = !gameBoy->soundController.channel2Master;
      }
      break;
    }
    case GLFW_KEY_3: { // Toggle sound channel 3
      if (action == 1) {
        gameBoy->soundController.channel3Master = !gameBoy->soundController.channel3Master;
      }
      break;
    }
    case GLFW_KEY_4: { // Toggle sound channel 4
      if (action == 1) {
        gameBoy->soundController.channel4Master = !gameBoy->soundController.channel4Master;
      }
      break;
    }
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

  // Create Game Boy state
  GameBoy gameBoy;
  Pixel frameBuffer[LCD_WIDTH * LCD_HEIGHT];
  AudioSampleBuffer audioSampleBuffer;

  // Load all cartridge data
  uint8_t* cartridgeData = cartridgeLoadData(argv[1]);
  if (cartridgeData == NULL) {
    error("Failed to read cartridge from '%s'\n", argv[1]);
    exit(EXIT_FAILURE);
  }

  sampleBufferInitialise(&audioSampleBuffer, 512 * 10); // CoreAudio requests buffers of 512 samples, so ten times that

  gbInitialise(&gameBoy, gameBoyType, cartridgeData, frameBuffer, romFilename);

  struct GBAudioContext* audioContext = initCoreAudioPlayback(&audioSampleBuffer);

  glfwSetErrorCallback(errorCallback);

  if (!glfwInit()) {
    error("Failed to initialise GLFW\n");
    exit(EXIT_FAILURE);
  }

  const int windowWidth = LCD_WIDTH * WINDOW_SCALE_FACTOR;
  const int windowHeight = LCD_HEIGHT * WINDOW_SCALE_FACTOR;

  // Prepare window title
  const char* windowTitlePrefix = EMULATOR_NAME " - ";
  char* windowTitle = (char*)malloc(strlen(windowTitlePrefix) + strlen(romFilename) + 1);
  strcpy(windowTitle, windowTitlePrefix);
  strcat(windowTitle, romFilename);

  GLFWwindow* window = glfwCreateWindow(windowWidth, windowHeight, windowTitle, NULL, NULL);
  if (!window) {
    error("Failed to initialise window or OpenGL context\n");
    glfwTerminate();
    exit(EXIT_FAILURE);
  }

  glfwMakeContextCurrent(window);

  GLFWmonitor* monitor = glfwGetPrimaryMonitor();
  if (!monitor) {
    error("Failed to get monitor\n");
    glfwTerminate();
    exit(EXIT_FAILURE);
  }

  const GLFWvidmode* videoMode = glfwGetVideoMode(monitor);
  if (!videoMode) {
    error("Failed to get video mode\n");
    glfwTerminate();
    exit(EXIT_FAILURE);
  }

  const int windowX = (videoMode->width - (LCD_WIDTH * WINDOW_SCALE_FACTOR)) / 2;
  const int windowY = (videoMode->height - (LCD_HEIGHT * WINDOW_SCALE_FACTOR)) / 2;

  glfwSetWindowPos(window, windowX, windowY);

  glfwSetWindowRefreshCallback(window, refreshCallback);
  glfwSetWindowSizeCallback(window, sizeCallback);
  glfwSetKeyCallback(window, keyCallback);

  // Prepare user data container that is made available to GLFW callbacks, so we can adjust GB settings
  struct UserData userData = {.gameBoy = &gameBoy, .frameBuffer = (Pixel*)&frameBuffer};
  glfwSetWindowUserPointer(window, &userData);

  glfwSwapInterval(1);

  setViewportAndProjection(window, windowWidth, windowHeight);

  lcdGLInit();
  lcdGLInitPixelVerticesArray();

  // GB display updates at ~59.7 frames per second, however as we're scheduling the emulator with the display of the
  // device it's running on we may have to run greater or fewer cycles per video frame than the value of
  // GBCyclesPerSec/GBFramesPerSec.
  // We obtain the nominal refresh rate directly from Core Video instead of using GLFW's GLFWvidmode as the refreshRate
  // value in the struct is an int while it could be a decimal value (represented accurately by CVTime) and is therefore
  // incorrectly truncated inside GLFW.
  const double refreshPeriod = cvDisplayLinkGetNominalOutputVideoRefreshPeriod();
  const int cyclesPerVideoFrame = (int)round(CLOCK_CYCLE_FREQUENCY_NORMAL_SPEED / refreshPeriod);

  debug("CVDisplayLink refresh period: %f\n", refreshPeriod);
  debug("GB cycles/video frame: %d\n", cyclesPerVideoFrame);

  int cyclesToRun = cyclesPerVideoFrame;
  while (!glfwWindowShouldClose(window)) {
    int cyclesRun = gbRunAtLeastNCycles(&gameBoy, &audioSampleBuffer, cyclesToRun);
    int extraCycles = cyclesRun - cyclesToRun;
    cyclesToRun = cyclesPerVideoFrame - extraCycles;

    lcdGLDrawScreen(frameBuffer);

    glfwSwapBuffers(window);
    glfwPollEvents();
  }

  glfwDestroyWindow(window);
  glfwTerminate();

  gbFinalise(&gameBoy);

  free((void*)audioContext);
  free((void*)windowTitle);
  free((void*)romFilename);
  free(cartridgeData);

  return 0;
}
