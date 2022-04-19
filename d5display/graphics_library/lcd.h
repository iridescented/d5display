/*  Author: Steve Gunn
 * Licence: This work is licensed under the Creative Commons Attribution License.
 *           View this license at http://creativecommons.org/about/licenses/
 */

#include <avr/io.h>
#include <stdint.h>
#include <util/delay.h>

#include "utils.h"

#define LCDWIDTH 240
#define LCDHEIGHT 320

/* Colour definitions RGB565 */
#define WHITE 0xFFFF
#define BLACK 0x0000
#define BLUE 0x001F
#define GREEN 0x07E0
#define CYAN 0x07FF
#define RED 0xF800
#define MAGENTA 0xF81F
#define YELLOW 0xFFE0

typedef uint16_t colour;

typedef enum { North,
               West,
               South,
               East } orientation;

typedef struct {
  uint16_t width, height;
  orientation orient;
  uint16_t x, y;
  uint16_t foreground, background;
} lcd;

extern lcd display;

typedef struct {
  uint16_t left, right;
  uint16_t top, bottom;
} rectangle;

void init_lcd();
void init_lcd_opts(orientation orn, colour fg, colour bg);
void lcd_brightness(uint8_t i);
void set_orientation(orientation o);
void set_frame_rate_hz(uint8_t f);
void clear_screen();
void fill_rectangle(rectangle r, uint16_t col);
void fill_rectangle_compat(coord pos, uint16_t width, uint16_t height, colour col);
void fill_rectangle_indexed(rectangle r, uint16_t *col);
void display_char(char c, colour fg, colour bg);
void display_string_xy(char *str, coord pos, colour fg, colour bg);
void lcd_reset();
void draw_pixel(coord pos, colour col);