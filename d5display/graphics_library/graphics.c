#include "graphics.h"

#include <stdlib.h>
/*
Initialise graphics library (basically just init lcd for now)
*/
void grInit(orientation orn, colour fg, colour bg) {
  init_lcd_opts(orn, fg, bg);
}

/*
Draw a rectangle outline with a given line width
*/
void grDrawRect(coord pos, uint16_t width, uint16_t height, uint8_t lineWidth, colour fg, colour bg) {
  if ((lineWidth >= width) | (lineWidth >= height)) {
    fill_rectangle_compat(pos, width, height, fg);
    return;
  }
  fill_rectangle_compat((coord){pos.x, pos.y}, width, height, bg);
  //Top rect
  fill_rectangle_compat((coord){pos.x, pos.y}, width, lineWidth, fg);
  //Bottom rect
  fill_rectangle_compat((coord){pos.x, (uint16_t)(pos.y + height - lineWidth)}, width, lineWidth, fg);
  //Left rect
  fill_rectangle_compat((coord){pos.x, pos.y}, lineWidth, height, fg);
  //Right rect
  fill_rectangle_compat((coord){(uint16_t)(pos.x + width - lineWidth), pos.y}, lineWidth, height, fg);
}

/*
Draw an outline of a circle using Bresenham's algorithm
*/
void grDrawCircle(coord pos, uint16_t radius, colour col) {
  int16_t x = 0;
  int16_t y = radius;
  int16_t decision = 3 - (2 * radius);
  DRAW_BRESENHAM_CIRCLE_FULL(pos.x, pos.y, x, y, col);
  while (y >= x) {
    x++;
    if (decision > 0) {
      y--;
      decision = decision + 4 * (x - y) + 10;
    } else {
      decision = decision + 4 * x + 6;
    }
    DRAW_BRESENHAM_CIRCLE_FULL(pos.x, pos.y, x, y, col);
  }
}

/*
Draw a horizontal line from `pos` with a given width
*/
void grDrawHLine(coord pos, uint16_t width, colour col) {
  fill_rectangle((rectangle){pos.x, (uint16_t)(pos.x + width), pos.y, pos.y}, col);
}

/*
Draw a vertical line from `pos` with a given height
*/
void grDrawVLine(coord pos, uint16_t height, colour col) {
  fill_rectangle((rectangle){pos.x, pos.x, pos.y, (uint16_t)(pos.y + height)}, col);
}

/*
Draw a string with parameterised forground and background colours
*/
void grWriteString(coord pos, char* string, colour fg, colour bg) {
  display_string_xy(string, pos, fg, bg);
}

/*
Clear display using function from LCD driver
*/
void grClearDisplay() {
  clear_screen();
}

/*
Draw a rectangle at a given position with given width and height
(Default colour is white)
*/
void grFillRect(coord pos, uint16_t width, uint16_t height, colour col) {
  fill_rectangle_compat(pos, width, height, col);
}

/*
Draw a corner of a circle (used as a helper for drawing other shapes):
*/
void grDrawCircleHelper(coord pos, uint16_t radius, uint8_t corner, colour col) {
  int16_t f = 1 - radius;
  int16_t decision_x = 1;
  int16_t decision_y = -2 * radius;
  int16_t x = 0;
  int16_t y = radius;

  while (x < y) {
    if (f >= 0) {
      y--;
      decision_y += 2;
      f += decision_y;
    }
    x++;
    decision_x += 2;
    f += decision_x;
    if (corner & 0x4) {  //Bottom right
      draw_pixel((coord){pos.x + x, pos.y + y}, col);
      draw_pixel((coord){pos.x + y, pos.y + x}, col);
    }
    if (corner & 0x2) {  //Top right
      draw_pixel((coord){pos.x + x, pos.y - y}, col);
      draw_pixel((coord){pos.x + y, pos.y - x}, col);
    }
    if (corner & 0x8) {  //Bottom left
      draw_pixel((coord){pos.x - y, pos.y + x}, col);
      draw_pixel((coord){pos.x - x, pos.y + y}, col);
    }
    if (corner & 0x1) {  //Top left
      draw_pixel((coord){pos.x - y, pos.y - x}, col);
      draw_pixel((coord){pos.x - x, pos.y - y}, col);
    }
  }
}

/*
Draw a filled circle
*/
void grFillCircle(coord pos, uint16_t radius, colour col) {
  grDrawVLine((coord){pos.x, pos.y - radius}, 2 * radius + 1, col);
  grFillCircleHelper(pos, radius, 0x3, 0, col);
}

/*
Helper function draw a filled circle
*/
void grFillCircleHelper(coord pos, uint16_t radius, uint8_t side, int16_t delta, colour col) {
  int16_t f = 1 - radius;
  int16_t decision_x = 1;
  int16_t decision_y = -2 * radius;
  int16_t x = 0;
  int16_t y = radius;

  while (x < y) {
    if (f >= 0) {
      y--;
      decision_y += 2;
      f += decision_y;
    }
    x++;
    decision_x += 2;
    f += decision_x;

    if (side & 0x1) {  //Left side
      grDrawVLine((coord){pos.x + x, pos.y - y}, 2 * y + 1 + delta, col);
      grDrawVLine((coord){pos.x + y, pos.y - x}, 2 * x + 1 + delta, col);
    }
    if (side & 0x2) {  //Right side
      grDrawVLine((coord){pos.x - x, pos.y - y}, 2 * y + 1 + delta, col);
      grDrawVLine((coord){pos.x - y, pos.y - x}, 2 * x + 1 + delta, col);
    }
  }
}

/*
Draw a line from one point to another
*/
void grDrawLine(coord p1, coord p2, colour color) {
  //Edge cases
  if (p1.y == p2.y) {
    if (p2.x > p1.x) {
      grDrawHLine((coord){p1.x, p1.y}, p2.x - p1.x + 1, color);
    } else if (p2.x < p1.x) {
      grDrawHLine((coord){p2.x, p1.y}, p1.x - p2.x + 1, color);
    } else {
      draw_pixel((coord){p1.x, p1.y}, color);
    }
    return;
  } else if (p1.x == p2.x) {
    if (p2.y > p1.y) {
      grDrawVLine((coord){p1.x, p1.y}, p2.y - p1.y + 1, color);
    } else {
      grDrawVLine((coord){p1.x, p2.y}, p1.y - p2.y + 1, color);
    }
    return;
  }

  char steep = abs(p2.y - p1.y) > abs(p2.x - p1.x);
  if (steep) {
    SWAP(p1.x, p1.y);
    SWAP(p2.x, p2.y);
  }
  if (p1.x > p2.x) {
    SWAP(p1.x, p2.x);
    SWAP(p1.y, p2.y);
  }

  int16_t dx, dy;
  dx = p2.x - p1.x;
  dy = abs(p2.y - p1.y);

  int16_t err = dx / 2;
  int16_t ystep;

  if (p1.y < p2.y) {
    ystep = 1;
  } else {
    ystep = -1;
  }

  int16_t xbegin = p1.x;
  if (steep) {
    for (; p1.x <= p2.x; p1.x++) {
      err -= dy;
      if (err < 0) {
        int16_t len = p1.x - xbegin;
        if (len) {
          grDrawVLine((coord){p1.y, xbegin}, len + 1, color);
        } else {
          draw_pixel((coord){p1.y, p1.x}, color);
        }
        xbegin = p1.x + 1;
        p1.y += ystep;
        err += dx;
      }
    }
    if ((int16_t)p1.x > xbegin + 1) {
      grDrawVLine((coord){p1.y, xbegin}, p1.x - xbegin, color);
    }
  } else {
    for (; p1.x <= p2.x; p1.x++) {
      err -= dy;
      if (err < 0) {
        int16_t len = p1.x - xbegin;
        if (len) {
          grDrawHLine((coord){xbegin, p1.y}, len + 1, color);
        } else {
          draw_pixel((coord){p1.x, p1.y}, color);
        }
        xbegin = p1.x + 1;
        p1.y += ystep;
        err += dx;
      }
    }
    if ((int16_t)p1.x > xbegin + 1) {
      grDrawHLine((coord){xbegin, p1.y}, p1.x - xbegin, color);
    }
  }
}

/*
Draw a rounded rectange
*/
void grDrawRoundedRect(coord pos, uint16_t width, uint16_t height, uint16_t radius, colour col) {
  //Top
  grDrawHLine((coord){pos.x + radius, pos.y}, width - 2 * radius, col);
  //Bottom
  grDrawHLine((coord){pos.x + radius, pos.y + height - 1}, width - 2 * radius, col);
  //Left
  grDrawVLine((coord){pos.x, pos.y + radius}, height - 2 * radius, col);
  //Right
  grDrawVLine((coord){pos.x + width - 1, pos.y + radius}, height - 2 * radius, col);

  //Draw corners
  grDrawCircleHelper((coord){pos.x + radius, pos.y + radius}, radius, 1, col);
  grDrawCircleHelper((coord){pos.x + width - radius - 1, pos.y + radius}, radius, 2, col);
  grDrawCircleHelper((coord){pos.x + width - radius - 1, pos.y + height - radius - 1}, radius, 4, col);
  grDrawCircleHelper((coord){pos.x + radius, pos.y + height - radius - 1}, radius, 8, col);
}

/*
Draw a filled rounded rect
*/
void grFillRoundedRect(coord pos, uint16_t width, uint16_t height, uint16_t radius, colour col) {
  grFillRect((coord){pos.x + radius, pos.y}, width - 2 * radius, height + 1, col);
  grFillCircleHelper((coord){pos.x + width - radius - 1, pos.y + radius}, radius, 1, height - 2 * radius - 1, col);
  grFillCircleHelper((coord){pos.x + radius, pos.y + radius}, radius, 2, height - 2 * radius - 1, col);
}

/*
Draw a triangle
*/
void grDrawTriangle(coord p1, coord p2, coord p3, colour col) {
  grDrawLine((coord){p1.x, p1.y}, (coord){p2.x, p2.y}, col);
  grDrawLine((coord){p2.x, p2.y}, (coord){p3.x, p3.y}, col);
  grDrawLine((coord){p3.x, p3.y}, (coord){p1.x, p1.y}, col);
}

/*
Draw a filled triangle
*/
void grFillTriangle(coord p1, coord p2, coord p3, colour color) {
  int16_t a, b, y, last;
  if (p1.y > p2.y) {
    SWAP(p1.y, p2.y);
    SWAP(p1.x, p2.x);
  }
  if (p2.y > p3.y) {
    SWAP(p3.y, p2.y);
    SWAP(p3.x, p2.x);
  }
  if (p1.y > p2.y) {
    SWAP(p1.y, p2.y);
    SWAP(p1.x, p2.x);
  }

  if (p1.y == p3.y) {
    a = b = p1.x;
    if ((int16_t)p2.x < a)
      a = p2.x;
    else if ((int16_t)p2.x > b)
      b = p2.x;
    if ((int16_t)p3.x < a)
      a = p3.x;
    else if ((int16_t)p3.x > b)
      b = p3.x;
    grDrawHLine((coord){a, p1.y}, b - a + 1, color);
    return;
  }

  int16_t
      dx01 = p2.x - p1.x,
      dy01 = p2.y - p1.y,
      dx02 = p3.x - p1.x,
      dy02 = p3.y - p1.y,
      dx12 = p3.x - p2.x,
      dy12 = p3.y - p2.y,
      sa = 0,
      sb = 0;

  if (p2.y == p3.y)
    last = p2.y;
  else
    last = p2.y - 1;

  for (y = p1.y; y <= last; y++) {
    a = p1.x + sa / dy01;
    b = p1.x + sb / dy02;
    sa += dx01;
    sb += dx02;
    if (a > b)
      SWAP(a, b);
    grDrawHLine((coord){a, y}, b - a + 1, color);
  }

  sa = dx12 * (y - p2.y);
  sb = dx02 * (y - p1.y);
  for (; y <= (int16_t)p3.y; y++) {
    a = p2.x + sa / dy12;
    b = p1.x + sb / dy02;
    sa += dx12;
    sb += dx02;
    if (a > b)
      SWAP(a, b);
    grDrawHLine((coord){a, y}, b - a + 1, color);
  }
}
