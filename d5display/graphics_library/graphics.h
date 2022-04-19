#include "lcd.h"
#define DRAW_BRESENHAM_CIRCLE_FULL(xc, yc, x, y, col) \
  draw_pixel((coord){(xc + x), (yc + y)}, col);       \
  draw_pixel((coord){(xc - x), (yc + y)}, col);       \
  draw_pixel((coord){(xc + x), (yc - y)}, col);       \
  draw_pixel((coord){(xc - x), (yc - y)}, col);       \
  draw_pixel((coord){(xc + y), (yc + x)}, col);       \
  draw_pixel((coord){(xc - y), (yc + x)}, col);       \
  draw_pixel((coord){(xc + y), (yc - x)}, col);       \
  draw_pixel((coord){(xc - y), (yc - x)}, col)
#define SWAP(a, b)   \
  {                  \
    typeof(a) t = a; \
    a = b;           \
    b = t;           \
  }

void grInit(orientation orn, colour fg, colour bg);
void grClearDisplay();
void grFillRect(coord pos, uint16_t width, uint16_t height, colour col);  //
void grWriteString(coord pos, char* string, colour fg, colour bg);
void grDrawRect(coord pos, uint16_t width, uint16_t height, uint8_t lineWidth, colour fg, colour bg);  //
void grDrawCircle(coord pos, uint16_t radius, colour col);                                             //
void grDrawHLine(coord pos, uint16_t width, colour col);
void grDrawVLine(coord pos, uint16_t height, colour col);
void grDrawCircleHelper(coord pos, uint16_t radius, uint8_t corner, colour col);
void grFillCircle(coord pos, uint16_t radius, colour col);  //
void grFillCircleHelper(coord pos, uint16_t radius, uint8_t corner, int16_t delta, colour col);
void grDrawLine(coord startPos, coord endPos, colour col);                                        //
void grDrawRoundedRect(coord pos, uint16_t width, uint16_t height, uint16_t radius, colour col);  //
void grFillRoundedRect(coord pos, uint16_t width, uint16_t height, uint16_t radius, colour col);  //
void grDrawTriangle(coord p1, coord p2, coord p3, colour col);
void grFillTriangle(coord p1, coord p2, coord p3, colour color);