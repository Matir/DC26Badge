#ifndef _HT16K33_H_
#define _HT16K33_H_

#include <stdint.h>

#include "nrfx_twim.h"

#define I2C_BUF_LEN 20

typedef struct _led_display {
  // TWI instance to use
  nrfx_twim_t *twi_instance;
  // Buffer for I2C Operations
  uint8_t buf[I2C_BUF_LEN];
  // Device address
  uint8_t addr;
} led_display;

extern uint16_t fontmap[128];

void init_led_display(led_display *disp, nrfx_twim_t *twi_instance,
    uint8_t addr);
ret_code_t display_text(led_display *disp, uint8_t *text);
ret_code_t display_set_brightness(led_display *disp, uint8_t level);
ret_code_t display_mode(led_display *disp, uint8_t on, uint8_t blink);
#define display_on(disp) display_mode((disp), 1, 0)
#define display_off(disp) display_mode((disp), 0, 0)

#endif /* _HT16K33_H_H */
