#include "lcdgl.h"

#include "lcd.h"
#include "pixel.h"

#include <GLUT/glut.h>


#define PIXEL_DATA_ARRAY_NUM_ELEMENTS_PER_VERTEX 2
#define PIXEL_DATA_ARRAY_NUM_ELEMENTS_PER_COLOUR 3

static float PIXEL_VERTICES[PIXEL_DATA_ARRAY_NUM_ELEMENTS * PIXEL_DATA_ARRAY_NUM_ELEMENTS_PER_VERTEX];
static float PIXEL_COLOURS[PIXEL_DATA_ARRAY_NUM_ELEMENTS * PIXEL_DATA_ARRAY_NUM_ELEMENTS_PER_COLOUR];


void lcdGLInitPixelVerticesArray()
{
  int i = 0;

  for (int y = 0; y < LCD_HEIGHT; y++) {
    for (int x = 0; x <= LCD_WIDTH; x++) {
      PIXEL_VERTICES[i++] = x;
      PIXEL_VERTICES[i++] = LCD_HEIGHT - y;
      PIXEL_VERTICES[i++] = x;
      PIXEL_VERTICES[i++] = LCD_HEIGHT - y - 1;
    }

    // Degenerate triangles
    PIXEL_VERTICES[i++] = LCD_WIDTH;
    PIXEL_VERTICES[i++] = LCD_HEIGHT - y - 1;
    PIXEL_VERTICES[i++] = 0;
    PIXEL_VERTICES[i++] = LCD_HEIGHT - y - 1;
  }
}


void lcdGLInit()
{
  glClearColor(0.0, 0.0, 0.0, 0.0);
  glEnable(GL_DEPTH_TEST);
  glShadeModel(GL_FLAT);

  glEnableClientState(GL_VERTEX_ARRAY);
  glEnableClientState(GL_COLOR_ARRAY);

  glVertexPointer(2, GL_FLOAT, 0, PIXEL_VERTICES);
  glColorPointer(3, GL_FLOAT, 0, PIXEL_COLOURS);
}


static void lcdGLFillColourArray(Pixel* frameBuffer)
{
  // With GL_FLAT and GL_TRIANGLE_STRIP we don't need to set r, g, and b values for the first two
  // colours of each line because only the last colour of each triangle determines the colour.
  // TODO: Eliminate duplication of colour values
  int i = 6;
  for (int y = 0; y < LCD_HEIGHT; y++) {
    for (int x = 0; x <= LCD_WIDTH; x++) {
      Pixel pixel = frameBuffer[y * LCD_WIDTH + x];
      PIXEL_COLOURS[i++] = pixel.r;
      PIXEL_COLOURS[i++] = pixel.g;
      PIXEL_COLOURS[i++] = pixel.b;
      PIXEL_COLOURS[i++] = pixel.r;
      PIXEL_COLOURS[i++] = pixel.g;
      PIXEL_COLOURS[i++] = pixel.b;
    }
    i += 6; // Skip past the two degenerate triangles because the colour doesn't matter
  }
}


void lcdGLDrawScreen(Pixel* frameBuffer)
{
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  glLoadIdentity();

  lcdGLFillColourArray(frameBuffer);

  glDrawArrays(GL_TRIANGLE_STRIP, 0, PIXEL_DATA_ARRAY_NUM_ELEMENTS);
}
