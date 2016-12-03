#ifndef LCDGL_H_
#define LCDGL_H_

#define PIXEL_DATA_ARRAY_NUM_ELEMENTS_PER_LINE (((LCD_WIDTH + 1) * 2) + 2)
// +1 for the final "fence post"
// *2 to specify top and bottom vertices
// +2 to add the extra vertices for the two degenerate triangles that allow us to jump to the next row (one on each side of the line at the same height)

#define PIXEL_DATA_ARRAY_NUM_ELEMENTS (PIXEL_DATA_ARRAY_NUM_ELEMENTS_PER_LINE * LCD_HEIGHT)


void lcdGLInitPixelVerticesArray();
void lcdGLInit();
void lcdGLDrawScreen();

#endif // LCDGL_H_
