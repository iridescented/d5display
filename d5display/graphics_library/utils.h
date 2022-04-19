#include <stdint.h>

/*
Convert HTML RGB to RGB565 uint16_t
*/

#define RGB_CONVERT(r, g, b) (uint16_t)(((r & 0xf8) << 8) + ((g & 0xfc) << 3) + (b >> 3))

/*
Screen coordinate struct used in various places
*/
typedef struct {
  uint16_t x, y;
} coord;