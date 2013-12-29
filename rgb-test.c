/** \file
 * Test the ledscape library by pulsing RGB on the first three LEDS.
 */
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <time.h>
#include <inttypes.h>
#include <errno.h>
#include <unistd.h>
#include "ledscape.h"

static void ledscape_fill_color(
  ledscape_frame_t * const frame,
  const unsigned num_pixels,
  const uint8_t r,
  const uint8_t g,
  const uint8_t b
)
{
  for (unsigned i = 0 ; i < num_pixels ; i++)
    for (unsigned strip = 0 ; strip < LEDSCAPE_NUM_STRIPS ; strip++)
      ledscape_set_color(frame, strip, i, r, g, b);
}

int main (void)
{
  const int num_pixels = 170;
  ledscape_t * const leds = ledscape_init(num_pixels);
  time_t last_time = time(NULL);
  unsigned last_i = 0;

  uint8_t rgb[3];

  unsigned i = 0;
  while (1)
  {
    // Alternate frame buffers on each draw command
    const unsigned frame_num = i++ % 2;
    ledscape_frame_t * const frame
      = ledscape_frame(leds, frame_num);

    uint8_t val = i >> 1;
    uint16_t r = ((i >>  0) & 0xFF);
    uint16_t g = ((i >>  4) & 0xFF);
    uint16_t b = ((i >>  8) & 0xFF);

    for (unsigned strip = 0 ; strip < LEDSCAPE_NUM_STRIPS ; strip++)
    {
      for (unsigned p = 0 ; p < num_pixels; p++)
      {
        HSBtoRGB(
          ((i + (p*360)/num_pixels) % 360), 
          100, 
          219,
          rgb
        );

        ledscape_set_color(
          frame,
          strip,
          p,
          
         rgb[0],
         rgb[1],
         rgb[2]
        );
        //ledscape_set_color(frame, strip, 3*p+1, 0, p+val + 80, 0);
        //ledscape_set_color(frame, strip, 3*p+2, 0, 0, p+val + 160);
      }
    }

    // wait for the previous frame to finish;
    const uint32_t response = ledscape_wait(leds);
    time_t now = time(NULL);

    if (now != last_time)
    {
      printf("%d fps. starting %d previous %"PRIx32"\n", i - last_i, i, response);
      last_i = i;
      last_time = now;
    }

    ledscape_draw(leds, frame_num);
  }

  ledscape_close(leds);

  return EXIT_SUCCESS;
}


//Gamma Correction Curve
const uint8_t dim_curve[] = {
    0,   1,   1,   2,   2,   2,   2,   2,   2,   3,   3,   3,   3,   3,   3,   3,
    3,   3,   3,   3,   3,   3,   3,   4,   4,   4,   4,   4,   4,   4,   4,   4,
    4,   4,   4,   5,   5,   5,   5,   5,   5,   5,   5,   5,   5,   6,   6,   6,
    6,   6,   6,   6,   6,   7,   7,   7,   7,   7,   7,   7,   8,   8,   8,   8,
    8,   8,   9,   9,   9,   9,   9,   9,   10,  10,  10,  10,  10,  11,  11,  11,
    11,  11,  12,  12,  12,  12,  12,  13,  13,  13,  13,  14,  14,  14,  14,  15,
    15,  15,  16,  16,  16,  16,  17,  17,  17,  18,  18,  18,  19,  19,  19,  20,
    20,  20,  21,  21,  22,  22,  22,  23,  23,  24,  24,  25,  25,  25,  26,  26,
    27,  27,  28,  28,  29,  29,  30,  30,  31,  32,  32,  33,  33,  34,  35,  35,
    36,  36,  37,  38,  38,  39,  40,  40,  41,  42,  43,  43,  44,  45,  46,  47,
    48,  48,  49,  50,  51,  52,  53,  54,  55,  56,  57,  58,  59,  60,  61,  62,
    63,  64,  65,  66,  68,  69,  70,  71,  73,  74,  75,  76,  78,  79,  81,  82,
    83,  85,  86,  88,  90,  91,  93,  94,  96,  98,  99,  101, 103, 105, 107, 109,
    110, 112, 114, 116, 118, 121, 123, 125, 127, 129, 132, 134, 136, 139, 141, 144,
    146, 149, 151, 154, 157, 159, 162, 165, 168, 171, 174, 177, 180, 183, 186, 190,
    193, 196, 200, 203, 207, 211, 214, 218, 222, 226, 230, 234, 238, 242, 248, 255,
};

void HSBtoRGB(int hue, int sat, int val, uint8_t out[]) { 
  /* convert hue, saturation and brightness ( HSB/HSV ) to RGB
     The dim_curve is used only on brightness/value and on saturation (inverted).
     This looks the most natural.      
  */

  val = dim_curve[val];
  sat = 255-dim_curve[255-sat];

  int r;
  int g;
  int b;
  int base;

  if (sat == 0) { // Acromatic color (gray). Hue doesn't mind.
    r = g = b = val;
  } else  { 
    base = ((255 - sat) * val)>>8;

    switch((hue%360)/60) {
    case 0:
        r = val;
        g = (((val-base)*hue)/60)+base;
        b = base;
    break;

    case 1:
        r = (((val-base)*(60-(hue%60)))/60)+base;
        g = val;
        b = base;
    break;

    case 2:
        r = base;
        g = val;
        b = (((val-base)*(hue%60))/60)+base;
    break;

    case 3:
        r = base;
        g = (((val-base)*(60-(hue%60)))/60)+base;
        b = val;
    break;

    case 4:
        r = (((val-base)*(hue%60))/60)+base;
        g = base;
        b = val;
    break;

    case 5:
        r = val;
        g = base;
        b = (((val-base)*(60-(hue%60)))/60)+base;
    break;
    }

    out[0] = r;
    out[1] = g;
    out[2] = b;
  }   
}
